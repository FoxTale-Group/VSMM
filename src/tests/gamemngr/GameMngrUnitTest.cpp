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

#include "GameMngrTestUtils.hpp"

#include <ConfigMock.hpp>
#include <GameMngr.hpp>

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

namespace {
// ways a configured exe can fail to yield a usable version
enum UnreadableVersion { NoExecutable, MissingExecutable, GarbageOutput, CrashingExecutable };
} // namespace

class GameMngrUnitTest : public QObject {
    Q_OBJECT

    static void ignoreEmptyGameExe() { QTest::ignoreMessage(QtCriticalMsg, "config paths.gameExe value is empty"); }
    // only gamemngr logs, and no tracing: no test pins a debug line
    static void onlyGameMngrLogs() {
        QLoggingCategory::setFilterRules(u"*=false\ngamemngr=true\ngamemngr.debug=false"_s);
    }

  private slots:
    void init() { onlyGameMngrLogs(); }

    void freshManagerHasNothing() {
        const vsmm::GameMngr gameMngr;

        QVERIFY(gameMngr.getModsDirs().isEmpty());
        QVERIFY(!gameMngr.getGameVersion());
    }

    void readsModPathsFromClientSettings() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(
            dir, clientSettings({u"Mods"_s, dir.absoluteFilePath(u"mods1"_s), dir.absoluteFilePath(u"mods2"_s)})));

        vsmm::ConfigMock config;
        // seeding before setConfig only populates state, nothing is connected yet
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        config.validate();

        QCOMPARE(spy.count(), 1);
        // "Mods" is the game's built-in folder, skip it and keep the order
        QCOMPARE(toPaths(gameMngr.getModsDirs()),
                 QStringList({dir.absoluteFilePath(u"mods1"_s), dir.absoluteFilePath(u"mods2"_s)}));
    }

    void configSetTwice() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(
            dir, clientSettings({u"Mods"_s, dir.absoluteFilePath(u"mods1"_s), dir.absoluteFilePath(u"mods2"_s)})));

        vsmm::ConfigMock config;
        vsmm::ConfigMock config2;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        config.validate();

        QTest::ignoreMessage(QtWarningMsg, "Config already set");
        gameMngr.setConfig(&config2);

        QCOMPARE(spy.count(), 1);
        // second config is not wired up, so it cannot drive a scan
        config2.validate();
        QCOMPARE(spy.count(), 1);
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
        QVERIFY(writeClientSettings(dir, clientSettings({entry, dir.absoluteFilePath(u"mods1"_s)})));

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);

        config.validate();

        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({dir.absoluteFilePath(u"mods1"_s)}));
    }

    void unreadableClientSettingsLeaveModsDirsEmpty_data() {
        QTest::addColumn<QByteArray>("contents");
        QTest::addColumn<bool>("writeTheFile");
        // each row pins why it gave up, so the diagnostics stay accurate
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
        QFETCH(bool, writeTheFile);
        QFETCH(QByteArray, expectedError);
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        if (writeTheFile) {
            QVERIFY(writeClientSettings(QDir{gameDir.path()}, contents));
        }

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        QTest::ignoreMessage(QtCriticalMsg, expectedError.constData());
        config.validate();

        QVERIFY(gameMngr.getModsDirs().isEmpty());
        // consumers still have to learn the scan produced nothing
        QCOMPARE(spy.count(), 1);
    }

    // unsupported settings file must not be parsed at all
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
        QVERIFY(writeClientSettings(QDir{gameDir.path()}, json));

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        QTest::ignoreMessage(QtCriticalMsg, expectedError.constData());
        config.validate();

        // modPaths are well-formed, so only the version gate can keep them out
        QVERIFY(gameMngr.getModsDirs().isEmpty());
        // rejected once, not rejected and then scanned anyway
        QCOMPARE(spy.count(), 1);
    }

    void switchingGameConfigDirReplacesModsDirs() {
        QTemporaryDir firstGameDir;
        QTemporaryDir secondGameDir;
        QVERIFY(firstGameDir.isValid());
        QVERIFY(secondGameDir.isValid());
        const QDir first{firstGameDir.path()};
        const QDir second{secondGameDir.path()};
        QVERIFY(writeClientSettings(first, clientSettings({first.absoluteFilePath(u"mods1"_s)})));
        QVERIFY(writeClientSettings(second, clientSettings({second.absoluteFilePath(u"mods2"_s)})));

        vsmm::ConfigMock config;
        config.setGameConfigDir(firstGameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};
        config.validate();
        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({first.absoluteFilePath(u"mods1"_s)}));

        // another game install replaces the dirs, it does not append to them
        config.setGameConfigDir(secondGameDir.path());

        QCOMPARE(spy.count(), 2);
        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({second.absoluteFilePath(u"mods2"_s)}));
    }

    void gameVersionIsReadFromTheGameExecutable() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(dir, clientSettings({})));
        const QString exe = writeFakeGameExe(dir, u"1.22.5"_s);
        QVERIFY(!exe.isEmpty());

        vsmm::ConfigMock config;
        config.setGamePaths(gameDir.path(), exe);

        vsmm::GameMngr gameMngr;
        gameMngr.setConfig(&config);
        // startup read is blocking, so the version is already there
        QVERIFY(gameMngr.getGameVersion());

        QCOMPARE(QString::fromStdString(gameMngr.getGameVersion()->to_string()), u"1.22.5"_s);
    }

    // half-set is worse than unknown, ModEntry matches no release and every mod then reads "no update"
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
        QVERIFY(writeClientSettings(dir, clientSettings({})));

        // each row pins its diagnostic, ignoreMessage fails when the message never arrives
        QString gameExe;
        switch (kind) {
        case NoExecutable:
            ignoreEmptyGameExe();
            break;
        case MissingExecutable:
            gameExe = dir.absoluteFilePath(u"not-installed"_s);
            // tail of this one is the platform's own errno text
            QTest::ignoreMessage(QtCriticalMsg,
                                 QRegularExpression{u"^Game exe failed while reading the version: .*\\(0\\)$"_s});
            break;
        case GarbageOutput:
            SKIP_WITHOUT_POSIX_SHELL();
            gameExe = writeFakeGameExe(dir, u"not-a-version"_s);
            QVERIFY(!gameExe.isEmpty());
            QTest::ignoreMessage(QtCriticalMsg, "Failed to parse game version");
            break;
        case CrashingExecutable:
            SKIP_WITHOUT_POSIX_SHELL();
            gameExe = writeCrashingExe(dir);
            QVERIFY(!gameExe.isEmpty());
            QTest::ignoreMessage(QtCriticalMsg, "Game exe failed while reading the version: Process crashed (1)");
            break;
        }

        vsmm::ConfigMock config;
        config.setGamePaths(gameDir.path(), gameExe);

        vsmm::GameMngr gameMngr;
        gameMngr.setConfig(&config);

        QVERIFY(!gameMngr.getGameVersion());
        QVERIFY(gameMngr.property("gameVersion").toString().isEmpty());
    }

    // exe set after startup must re-read the version, non-blocking so a Settings save cannot freeze the UI
    void settingGameExeLaterReReadsTheVersionAndNotifies() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(dir, clientSettings({})));

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::gameVersionChanged};
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QVERIFY(!gameMngr.getGameVersion());
        // unknown was already the starting state, so nothing to notify
        QCOMPARE(spy.count(), 0);

        const QString exe = writeFakeGameExe(dir, u"1.22.5"_s);
        QVERIFY(!exe.isEmpty());
        config.setGameExePath(exe);

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
        QVERIFY(writeClientSettings(dir, clientSettings({})));
        const QString exe = writeFakeGameExe(dir, u"1.22.5"_s);
        QVERIFY(!exe.isEmpty());

        vsmm::ConfigMock config;
        config.setGamePaths(gameDir.path(), exe);

        vsmm::GameMngr gameMngr;
        gameMngr.setConfig(&config);
        QVERIFY(gameMngr.getGameVersion());
        QCOMPARE(gameMngr.property("gameVersion").toString(), u"1.22.5"_s);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::gameVersionChanged};

        // user repoints the app at an exe that is gone
        QTest::ignoreMessage(QtCriticalMsg,
                             QRegularExpression{u"^Game exe failed while reading the version: .*\\(0\\)$"_s});
        config.setGameExePath(dir.absoluteFilePath(u"gone"_s));

        QTRY_VERIFY(!gameMngr.getGameVersion());
        QVERIFY(gameMngr.property("gameVersion").toString().isEmpty());
        QCOMPARE(spy.count(), 1);
    }

    // mods dirs stay the same, but scanned mods were update checked against the old version, so rescan
    void aChangedVersionAloneTriggersARescan() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(dir, clientSettings({dir.absoluteFilePath(u"mods1"_s)})));

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        config.validate();

        QSignalSpy scanSpy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        const QString exe = writeFakeGameExe(dir, u"1.22.5"_s);
        QVERIFY(!exe.isEmpty());
        config.setGameExePath(exe);

        QTRY_COMPARE(scanSpy.count(), 1);
        QVERIFY(gameMngr.getGameVersion());
    }

    // update detection is never recomputed, so no scan while a version read is still resolving
    void noScanStartsBeforeTheVersionReadSettles() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir firstGameDir;
        QTemporaryDir secondGameDir;
        QVERIFY(firstGameDir.isValid());
        QVERIFY(secondGameDir.isValid());
        const QDir first{firstGameDir.path()};
        const QDir second{secondGameDir.path()};
        QVERIFY(writeClientSettings(first, clientSettings({first.absoluteFilePath(u"mods1"_s)})));
        QVERIFY(writeClientSettings(second, clientSettings({second.absoluteFilePath(u"mods2"_s)})));

        vsmm::ConfigMock config;
        config.setGameConfigDir(firstGameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        config.validate();

        QSignalSpy scanSpy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};
        bool versionKnownAtScan = false;
        connect(&gameMngr, &vsmm::GameMngr::modsDirsChanged, this,
                [&] { versionKnownAtScan = gameMngr.getGameVersion().has_value(); });

        const QString exe = writeFakeGameExe(second, u"1.22.5"_s);
        QVERIFY(!exe.isEmpty());
        // one Settings save changing both paths at once
        config.setGamePaths(secondGameDir.path(), exe);

        // version read still in flight, so no scan announced yet
        QCOMPARE(scanSpy.count(), 0);

        QTRY_COMPARE(scanSpy.count(), 1);
        QVERIFY2(versionKnownAtScan, "scan was announced while the game version was still unknown");
        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({second.absoluteFilePath(u"mods2"_s)}));
    }

    // version probe and launch use separate QProcess objects, so a busy probe cannot block a launch
    void launchWorksWhileAVersionReadIsInFlight() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        const QString marker = dir.absoluteFilePath(u"launched"_s);
        QVERIFY(writeClientSettings(dir, clientSettings({})));

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);

        // async read that will not finish on its own
        const QString hanging = writeHangingExe(dir);
        QVERIFY(!hanging.isEmpty());
        config.setGameExePath(hanging);

        // second change is ignored while that read is in flight, so the probe stays busy
        const QString launcher = writeFakeGameExe(dir, u"1.22.5"_s, marker);
        QVERIFY(!launcher.isEmpty());
        // the warning is the only evidence of that
        QTest::ignoreMessage(QtWarningMsg, "Game version read already in progress");
        config.setGameExePath(launcher);

        gameMngr.launchGame();

        QTRY_VERIFY(QFile::exists(marker));
    }

    // slow by design, waits out the real 3 s probe timeout
    void aTimedOutVersionReadIsKilledAndRecoverable() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(dir, clientSettings({})));
        const QString hanging = writeHangingExe(dir);
        QVERIFY(!hanging.isEmpty());

        vsmm::ConfigMock config;
        config.setGamePaths(gameDir.path(), hanging);

        vsmm::GameMngr gameMngr;
        QTest::ignoreMessage(QtCriticalMsg, "Game exe did not exit for --version; is paths.gameExe the right binary?");
        gameMngr.setConfig(&config);

        QElapsedTimer timer;
        timer.start();
        QVERIFY(!gameMngr.getGameVersion());
        // guards against falling back to QProcess 30 s default
        QVERIFY2(timer.elapsed() < 10000, qPrintable(QString::number(timer.elapsed())));

        // timed out probe must be killed, or every later read gets refused
        const QString exe = writeFakeGameExe(dir, u"1.22.5"_s);
        QVERIFY(!exe.isEmpty());
        config.setGameExePath(exe);

        QTRY_COMPARE(gameMngr.property("gameVersion").toString(), u"1.22.5"_s);
    }

    void launchWithoutAGameExecutableIsANoOp() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        QVERIFY(writeClientSettings(QDir{gameDir.path()}, clientSettings({})));

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

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
        QVERIFY(writeClientSettings(dir, clientSettings({})));
        const QString exe = writeFakeGameExe(dir, u"1.22.5"_s, marker);
        QVERIFY(!exe.isEmpty());

        vsmm::ConfigMock config;
        config.setGamePaths(gameDir.path(), exe);

        vsmm::GameMngr gameMngr;
        gameMngr.setConfig(&config);
        // no extra version read, launchGame must set up its own process instead of reusing mVersionProcess
        QVERIFY(!QFile::exists(marker));

        gameMngr.launchGame();

        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(marker), 5000);
    }
};

QTEST_GUILESS_MAIN(GameMngrUnitTest)
#include "GameMngrUnitTest.moc"
