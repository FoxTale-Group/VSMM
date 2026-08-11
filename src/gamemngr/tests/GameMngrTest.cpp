/*
 * VSMM - A mod management tool for Vintage Story
 * Copyright (C) 2026 FoxTale-Group VSMM Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <GameMngr.hpp>

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

// The fake game executables are POSIX shell scripts, which CreateProcess cannot run directly.
#ifdef Q_OS_WIN
#define SKIP_WITHOUT_POSIX_SHELL() QSKIP("Uses a POSIX shell script as a stand-in for the game executable")
#else
#define SKIP_WITHOUT_POSIX_SHELL() ((void)0)
#endif

namespace {

constexpr QLatin1StringView SUPPORTED_SETTINGS_VERSION{"1.16"};
constexpr QLatin1StringView CLIENT_SETTINGS_FILE{"clientsettings.json"};

[[nodiscard]] QJsonObject clientSettings(const QJsonArray &modPaths,
                                         QLatin1StringView settingsVersion = SUPPORTED_SETTINGS_VERSION) {
    return {{"stringSettings"_L1, QJsonObject{{"settingsVersion"_L1, settingsVersion}}},
            {"stringListSettings"_L1, QJsonObject{{"modPaths"_L1, modPaths}}}};
}

// The ways a configured executable can fail to yield a usable version.
enum UnreadableVersion { NoExecutable, MissingExecutable, GarbageOutput, CrashingExecutable };

[[nodiscard]] QStringList toPaths(const QList<QDir> &dirs) {
    QStringList paths;
    paths.reserve(dirs.size());
    for (const auto &dir : dirs) {
        paths.append(dir.path());
    }
    return paths;
}

} // namespace

class GameMngrTest : public QObject {
    Q_OBJECT

    [[nodiscard]] static QString configDir() {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    [[nodiscard]] static QString configFilePath() { return configDir() + QDir::separator() + "config.json"_L1; }

    static void writeFile(const QString &path, const QByteArray &contents) {
        QFile file{path};
        QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate), qPrintable(path));
        QCOMPARE(file.write(contents), contents.size());
    }

    // config.json as the app would find it on disk. `gameExe` is optional, most tests do not need it.
    static void writeConfig(const QString &gameConfigDir, const QString &gameExe = {}) {
        QVERIFY(QDir().mkpath(configDir()));
        QJsonObject paths{{"gameConfig"_L1, gameConfigDir}};
        if (!gameExe.isEmpty()) {
            paths["gameExe"_L1] = gameExe;
        }
        writeFile(configFilePath(),
                  QJsonDocument{
                      QJsonObject{{"general"_L1, QJsonObject{}}, {"appearance"_L1, QJsonObject{}}, {"paths"_L1, paths}}}
                      .toJson());
    }

    static void writeClientSettings(const QDir &gameDir, const QByteArray &contents) {
        writeFile(gameDir.absoluteFilePath(CLIENT_SETTINGS_FILE), contents);
    }

    static void writeClientSettings(const QDir &gameDir, const QJsonObject &json) {
        writeClientSettings(gameDir, QJsonDocument{json}.toJson());
    }

    // Stand-in for the game binary: prints a version for `--version`, otherwise touches `marker`
    // so a launch can be observed. POSIX only, the launch tests skip elsewhere.
    static QString writeFakeGameExe(const QDir &dir, const QString &version, const QString &marker = {}) {
        const QString path = dir.absoluteFilePath("fake-vintagestory.sh"_L1);
        const QByteArray script = "#!/bin/sh\n"
                                  "if [ \"$1\" = \"--version\" ]; then\n"
                                  "  echo " +
                                  version.toUtf8() +
                                  "\n"
                                  "else\n"
                                  "  touch \"" +
                                  marker.toUtf8() +
                                  "\"\n"
                                  "fi\n";
        writeFile(path, script);
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        return path;
    }

    // A binary that starts fine and then dies on a signal.
    static QString writeCrashingExe(const QDir &dir) {
        const QString path = dir.absoluteFilePath("crashing.sh"_L1);
        writeFile(path, QByteArray{"#!/bin/sh\nkill -SEGV $$\n"});
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        return path;
    }

    // A binary that starts fine but never exits, standing in for one that ignores `--version`.
    static QString writeHangingExe(const QDir &dir) {
        const QString path = dir.absoluteFilePath("hanging.sh"_L1);
        writeFile(path, QByteArray{"#!/bin/sh\nsleep 30\n"});
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        return path;
    }

    static void ignoreEmptyGameExe() { QTest::ignoreMessage(QtCriticalMsg, "config paths.gameExe value is empty"); }
    static void ignoreVersionDetected() { QTest::ignoreMessage(QtDebugMsg, "Game version detected: 1.22.5"); }
    static void ignoreProcessStarted() {
        QTest::ignoreMessage(QtDebugMsg, QRegularExpression{u"^Process started as \\d+$"_s});
    }

  private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(configDir().contains("qttest"_L1), qPrintable(configDir()));
    }

    void cleanupTestCase() {
        QFile::remove(configFilePath());
        QStandardPaths::setTestModeEnabled(false);
    }

    void freshManagerHasNothing() {
        const vsmm::GameMngr gameMngr;

        QVERIFY(gameMngr.getModsDirs().isEmpty());
        QVERIFY(!gameMngr.getGameVersion());
    }

    void readsModPathsFromClientSettings() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        writeClientSettings(
            dir, clientSettings({u"Mods"_s, dir.absoluteFilePath(u"mods1"_s), dir.absoluteFilePath(u"mods2"_s)}));
        writeConfig(gameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        config.validate();

        QCOMPARE(spy.count(), 1);
        // "Mods" is the game's built-in folder and must be skipped; order is preserved.
        QCOMPARE(toPaths(gameMngr.getModsDirs()),
                 QStringList({dir.absoluteFilePath(u"mods1"_s), dir.absoluteFilePath(u"mods2"_s)}));
    }

    void configSetTwice() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        writeClientSettings(
            dir, clientSettings({u"Mods"_s, dir.absoluteFilePath(u"mods1"_s), dir.absoluteFilePath(u"mods2"_s)}));
        writeConfig(gameDir.path());

        vsmm::Config config;
        vsmm::Config config2;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        config.validate();

        QTest::ignoreMessage(QtCriticalMsg, "Config already set");
        gameMngr.setConfig(&config2);

        QCOMPARE(spy.count(), 1);
        // "Mods" is the game's built-in folder and must be skipped; order is preserved.
        QCOMPARE(toPaths(gameMngr.getModsDirs()),
                 QStringList({dir.absoluteFilePath(u"mods1"_s), dir.absoluteFilePath(u"mods2"_s)}));
    }

    void skipsUnusableModPathEntries_data() {
        QTest::addColumn<QJsonValue>("entry");
        QTest::newRow("built-in-Mods") << QJsonValue{"Mods"_L1};
        QTest::newRow("null") << QJsonValue{QJsonValue::Null};
        QTest::newRow("empty-string") << QJsonValue{""_L1};
        QTest::newRow("not-a-string") << QJsonValue{42};
    }

    void skipsUnusableModPathEntries() {
        QFETCH(QJsonValue, entry);
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        writeClientSettings(dir, clientSettings({entry, dir.absoluteFilePath(u"mods1"_s)}));
        writeConfig(gameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);

        config.validate();

        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({dir.absoluteFilePath(u"mods1"_s)}));
    }

    void unreadableClientSettingsLeaveModsDirsEmpty_data() {
        QTest::addColumn<QByteArray>("contents");
        QTest::addColumn<bool>("writeFile");
        // Each row must also say why it gave up, so the diagnostics stay accurate.
        QTest::addColumn<QByteArray>("expectedError");
        QTest::newRow("file-missing") << QByteArray{} << false << QByteArray{"Client settings file does not exist"};
        QTest::newRow("not-json") << QByteArray{"not json at all"} << true
                                  << QByteArray{"Client settings file is not a valid JSON object"};
        QTest::newRow("json-array") << QByteArray{"[1, 2, 3]"} << true
                                    << QByteArray{"Client settings file is not a valid JSON object"};
        QTest::newRow("stringListSettings-missing")
            << QJsonDocument{QJsonObject{{"stringSettings"_L1,
                                          QJsonObject{{"settingsVersion"_L1, SUPPORTED_SETTINGS_VERSION}}}}}
                   .toJson()
            << true << QByteArray{"Failed to locate stringListSettings in client settings file"};
        QTest::newRow("modPaths-not-an-array")
            << QJsonDocument{QJsonObject{
                                 {"stringSettings"_L1, QJsonObject{{"settingsVersion"_L1, SUPPORTED_SETTINGS_VERSION}}},
                                 {"stringListSettings"_L1, QJsonObject{{"modPaths"_L1, "/opt/mods"_L1}}}}}
                   .toJson()
            << true << QByteArray{"Invalid modPaths"};
    }

    void unreadableClientSettingsLeaveModsDirsEmpty() {
        QFETCH(QByteArray, contents);
        QFETCH(bool, writeFile);
        QFETCH(QByteArray, expectedError);
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        if (writeFile) {
            writeClientSettings(QDir{gameDir.path()}, contents);
        }
        writeConfig(gameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        QTest::ignoreMessage(QtCriticalMsg, expectedError.constData());
        config.validate();

        QVERIFY(gameMngr.getModsDirs().isEmpty());
        // The consumers still have to learn the scan produced nothing.
        QCOMPARE(spy.count(), 1);
    }

    // A settings file the app does not understand must not be parsed at all.
    void unsupportedClientSettingsAreRejected_data() {
        QTest::addColumn<QJsonObject>("json");
        QTest::addColumn<QByteArray>("expectedError");
        QTest::newRow("unsupported-version")
            << clientSettings({u"/opt/vs/mods1"_s}, "1.15"_L1)
            << QByteArray{"Detected unsupported clientsettings: Unsupported clientsettings version 1.15"};
        QTest::newRow("stringSettings-missing")
            << QJsonObject{{"stringListSettings"_L1, QJsonObject{{"modPaths"_L1, QJsonArray{u"/opt/vs/mods1"_s}}}}}
            << QByteArray{"Detected unsupported clientsettings: Invalid stringSettings"};
    }

    void unsupportedClientSettingsAreRejected() {
        QFETCH(QJsonObject, json);
        QFETCH(QByteArray, expectedError);
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        writeClientSettings(QDir{gameDir.path()}, json);
        writeConfig(gameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        QTest::ignoreMessage(QtCriticalMsg, expectedError.constData());
        config.validate();

        // The modPaths in the file are well-formed, so only the version gate can keep them out.
        QVERIFY(gameMngr.getModsDirs().isEmpty());
        // Rejected once, not rejected and then scanned anyway.
        QCOMPARE(spy.count(), 1);
    }

    void switchingGameConfigDirReplacesModsDirs() {
        QTemporaryDir firstGameDir;
        QTemporaryDir secondGameDir;
        QVERIFY(firstGameDir.isValid());
        QVERIFY(secondGameDir.isValid());
        const QDir first{firstGameDir.path()};
        const QDir second{secondGameDir.path()};
        writeClientSettings(first, clientSettings({first.absoluteFilePath(u"mods1"_s)}));
        writeClientSettings(second, clientSettings({second.absoluteFilePath(u"mods2"_s)}));
        writeConfig(firstGameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};
        config.validate();
        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({first.absoluteFilePath(u"mods1"_s)}));

        // Pointing the app at another game install replaces the dirs, it does not append to them.
        QVERIFY(config.setProperty("paths", QVariantHash{{u"gameConfig"_s, secondGameDir.path()}}));

        QCOMPARE(spy.count(), 2);
        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({second.absoluteFilePath(u"mods2"_s)}));
    }

    void gameVersionIsReadFromTheGameExecutable() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        writeClientSettings(dir, clientSettings({}));
        writeConfig(gameDir.path(), writeFakeGameExe(dir, u"1.22.5"_s));

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreVersionDetected();
        gameMngr.setConfig(&config);
        // Reading the version is an explicit step; setConfig only wires signals up.
        QVERIFY(gameMngr.getGameVersion());

        QCOMPARE(QString::fromStdString(gameMngr.getGameVersion()->to_string()), u"1.22.5"_s);
    }

    // Every way a version read can fail must leave it unknown rather than half-set, because
    // ModEntry matches no release against a default version and every mod then reads "no update".
    void versionStaysUnknownWhenItCannotBeRead_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("no-executable-configured") << int(NoExecutable);
        QTest::newRow("executable-missing") << int(MissingExecutable);
        QTest::newRow("output-is-not-a-version") << int(GarbageOutput);
        QTest::newRow("executable-crashes") << int(CrashingExecutable);
    }

    void versionStaysUnknownWhenItCannotBeRead() {
        QFETCH(int, kind);
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        writeClientSettings(dir, clientSettings({}));

        // Each row must also produce the diagnostic that explains it; ignoreMessage() fails the
        // test if the message is missing, so the wording is pinned, not merely silenced.
        QString gameExe;
        switch (kind) {
        case NoExecutable:
            QTest::ignoreMessage(QtCriticalMsg, "config paths.gameExe value is empty");
            break;
        case MissingExecutable:
            gameExe = dir.absoluteFilePath(u"not-installed"_s);
            // The tail of this one is the platform's own errno text.
            QTest::ignoreMessage(QtCriticalMsg,
                                 QRegularExpression{u"^Game exe failed while reading the version: .*\\(0\\)$"_s});
            break;
        case GarbageOutput:
            SKIP_WITHOUT_POSIX_SHELL();
            gameExe = writeFakeGameExe(dir, u"not-a-version"_s);
            QTest::ignoreMessage(QtCriticalMsg, "Failed to parse game version");
            break;
        case CrashingExecutable:
            SKIP_WITHOUT_POSIX_SHELL();
            gameExe = writeCrashingExe(dir);
            QTest::ignoreMessage(QtCriticalMsg, "Game exe failed while reading the version: Process crashed (1)");
            break;
        }
        writeConfig(gameDir.path(), gameExe);

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        gameMngr.setConfig(&config);

        QVERIFY(!gameMngr.getGameVersion());
        QVERIFY(gameMngr.property("gameVersion").toString().isEmpty());
    }

    // Configuring the exe after startup must re-read the version, otherwise it stays unknown for
    // the whole session and ModEntry matches no release against it. The read is non-blocking so a
    // Settings save does not freeze the UI, and the QML-facing property follows it.
    void settingGameExeLaterReReadsTheVersionAndNotifies() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        writeClientSettings(dir, clientSettings({}));
        writeConfig(gameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::gameVersionChanged};
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QVERIFY(!gameMngr.getGameVersion());
        // Unknown was already the starting state, so there was nothing to notify.
        QCOMPARE(spy.count(), 0);

        const QString exe = writeFakeGameExe(dir, u"1.22.5"_s);
        ignoreVersionDetected();
        QVERIFY(config.setProperty("paths", QVariantHash{{u"gameConfig"_s, gameDir.path()}, {u"gameExe"_s, exe}}));

        QTRY_VERIFY(gameMngr.getGameVersion());
        QCOMPARE(QString::fromStdString(gameMngr.getGameVersion()->to_string()), u"1.22.5"_s);
        QCOMPARE(gameMngr.property("gameVersion").toString(), u"1.22.5"_s);
        QCOMPARE(spy.count(), 1);
    }

    void unreadableVersionClearsAPreviouslyKnownVersion() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        writeClientSettings(dir, clientSettings({}));
        writeConfig(gameDir.path(), writeFakeGameExe(dir, u"1.22.5"_s));

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreVersionDetected();
        gameMngr.setConfig(&config);
        QVERIFY(gameMngr.getGameVersion());
        QCOMPARE(gameMngr.property("gameVersion").toString(), u"1.22.5"_s);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::gameVersionChanged};

        // The user repoints the app at an executable that is not there any more.
        QTest::ignoreMessage(QtCriticalMsg,
                             QRegularExpression{u"^Game exe failed while reading the version: .*\\(0\\)$"_s});
        QVERIFY(config.setProperty(
            "paths", QVariantHash{{u"gameConfig"_s, gameDir.path()}, {u"gameExe"_s, dir.absoluteFilePath(u"gone"_s)}}));

        QTRY_VERIFY(!gameMngr.getGameVersion());
        QVERIFY(gameMngr.property("gameVersion").toString().isEmpty());
        QCOMPARE(spy.count(), 1);
    }

    // Update detection runs per mod against the version of the moment and is never recomputed, so
    // a scan must not start while a version read is still resolving.
    void noScanStartsBeforeTheVersionReadSettles() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir firstGameDir;
        QTemporaryDir secondGameDir;
        QVERIFY(firstGameDir.isValid());
        QVERIFY(secondGameDir.isValid());
        const QDir first{firstGameDir.path()};
        const QDir second{secondGameDir.path()};
        writeClientSettings(first, clientSettings({first.absoluteFilePath(u"mods1"_s)}));
        writeClientSettings(second, clientSettings({second.absoluteFilePath(u"mods2"_s)}));
        writeConfig(firstGameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        config.validate();

        QSignalSpy scanSpy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};
        bool versionKnownAtScan = false;
        connect(&gameMngr, &vsmm::GameMngr::modsDirsChanged, this,
                [&] { versionKnownAtScan = gameMngr.getGameVersion().has_value(); });

        // One Settings save changing both paths at once.
        ignoreVersionDetected();
        QVERIFY(config.setProperty("paths", QVariantHash{{u"gameConfig"_s, secondGameDir.path()},
                                                         {u"gameExe"_s, writeFakeGameExe(second, u"1.22.5"_s)}}));

        // The version read is still in flight, so the scan must not have been announced yet.
        QCOMPARE(scanSpy.count(), 0);

        QTRY_COMPARE(scanSpy.count(), 1);
        QVERIFY2(versionKnownAtScan, "scan was announced while the game version was still unknown");
        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({second.absoluteFilePath(u"mods2"_s)}));
    }

    // Changing only the exe leaves the mods dirs alone, but every already-scanned mod was update
    // checked against the old version, so one rescan is still owed.
    void aChangedVersionAloneTriggersARescan() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        writeClientSettings(dir, clientSettings({dir.absoluteFilePath(u"mods1"_s)}));
        writeConfig(gameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        config.validate();

        QSignalSpy scanSpy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        QVariantHash paths = config.property("paths").toHash();
        paths[u"gameExe"_s] = writeFakeGameExe(dir, u"1.22.5"_s);
        ignoreVersionDetected();
        QVERIFY(config.setProperty("paths", paths));

        QTRY_COMPARE(scanSpy.count(), 1);
        QVERIFY(gameMngr.getGameVersion());
    }

    // The version probe and the game launch use separate QProcess objects, so a probe that is
    // still running (or was killed on timeout) can never stop the user from launching.
    void launchWorksWhileAVersionReadIsInFlight() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        const QString marker = dir.absoluteFilePath(u"launched"_s);
        writeClientSettings(dir, clientSettings({}));
        writeConfig(gameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);

        // Start an async read that will not finish on its own.
        QVERIFY(config.setProperty(
            "paths", QVariantHash{{u"gameConfig"_s, gameDir.path()}, {u"gameExe"_s, writeHangingExe(dir)}}));
        // A second change is ignored while that read is in flight, so the probe stays busy.
        QTest::ignoreMessage(QtDebugMsg, "Game version read already in progress");
        QVERIFY(config.setProperty("paths", QVariantHash{{u"gameConfig"_s, gameDir.path()},
                                                         {u"gameExe"_s, writeFakeGameExe(dir, u"1.22.5"_s, marker)}}));

        ignoreProcessStarted();
        gameMngr.launchGame();

        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(marker), 5000);
    }

    // Slow by design: it waits out the real 3 s probe timeout.
    void aTimedOutVersionReadIsKilledAndRecoverable() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        writeClientSettings(dir, clientSettings({}));
        writeConfig(gameDir.path(), writeHangingExe(dir));

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        QTest::ignoreMessage(QtCriticalMsg, "Game exe did not exit for --version; is paths.gameExe the right binary?");
        gameMngr.setConfig(&config);

        QElapsedTimer timer;
        timer.start();
        QVERIFY(!gameMngr.getGameVersion());
        // Guards against falling back to QProcess's 30 s default.
        QVERIFY2(timer.elapsed() < 10000, qPrintable(QString::number(timer.elapsed())));

        // The timed-out probe must have been killed, or every later read would be refused.
        ignoreVersionDetected();
        QVERIFY(config.setProperty("paths", QVariantHash{{u"gameConfig"_s, gameDir.path()},
                                                         {u"gameExe"_s, writeFakeGameExe(dir, u"1.22.5"_s)}}));

        QTRY_COMPARE(gameMngr.property("gameVersion").toString(), u"1.22.5"_s);
    }

    void launchWithoutAGameExecutableIsANoOp() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        writeClientSettings(QDir{gameDir.path()}, clientSettings({}));
        writeConfig(gameDir.path());

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QVERIFY(!gameMngr.getGameVersion());

        ignoreEmptyGameExe();
        gameMngr.launchGame();

        QVERIFY(!QFile::exists(QDir{gameDir.path()}.absoluteFilePath(u"launched"_s)));
    }

    void launchStartsTheConfiguredExecutable() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        const QString marker = dir.absoluteFilePath(u"launched"_s);
        writeClientSettings(dir, clientSettings({}));
        writeConfig(gameDir.path(), writeFakeGameExe(dir, u"1.22.5"_s, marker));

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreVersionDetected();
        gameMngr.setConfig(&config);
        // Deliberately no readGameVersion() call: launchGame() must set up the process itself
        // rather than reuse whatever a version read happened to leave on mGameProcess.
        QVERIFY(!QFile::exists(marker));

        ignoreProcessStarted();
        gameMngr.launchGame();

        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(marker), 5000);
    }
};

QTEST_GUILESS_MAIN(GameMngrTest)
#include "GameMngrTest.moc"
