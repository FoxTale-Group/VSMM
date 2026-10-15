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

#include <GameMngrTestUtils.hpp>

#include <ConfigMock.hpp>
#include <GameMngr.hpp>

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

namespace {
// ways a configured exe can fail to yield a usable version
enum UnreadableVersion { NoExecutable, MissingExecutable, GarbageOutput, CrashingExecutable };

// game dir with a clientsettings.json the manager already read
struct LoadedGameMngr {
    QTemporaryDir gameDir;
    vsmm::ConfigMock config;
    vsmm::GameMngr gameMngr;

    [[nodiscard]] QDir dir() const { return QDir{gameDir.path()}; }
    [[nodiscard]] QString settingsPath() const { return QDir::cleanPath(dir().filePath(CLIENT_SETTINGS_FILE)); }
    [[nodiscard]] QJsonArray modPaths() const { return {dir().absoluteFilePath(u"mods1"_s)}; }

    // false when the file could not be written
    [[nodiscard]] bool load(const QJsonObject &settings) {
        if (!gameDir.isValid() || !writeClientSettings(dir(), settings) || !backdate(settingsPath())) {
            return false;
        }
        config.setGameConfigDir(gameDir.path());
        QTest::ignoreMessage(QtCriticalMsg, "config paths.gameExe value is empty");
        gameMngr.setConfig(&config);
        // every good read logs the list, readDisabledModsAreLogged pins the text
        if (settings["stringListSettings"_L1]["disabledMods"_L1].isArray()) {
            QTest::ignoreMessage(QtInfoMsg, QRegularExpression{u"^(Disabled mods \\(\\d+\\): |No disabled mods$)"_s});
        }
        config.validate();
        return true;
    }
    [[nodiscard]] bool load(const QJsonArray &disabledMods) { return load(clientSettings(modPaths(), disabledMods)); }
};

// paths from "Client settings file changed", the only sign a watcher event was handled
QStringList gChangedPaths;
QtMessageHandler gPreviousHandler = nullptr;

void recordChangedPaths(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    if (const QLatin1StringView prefix{"Client settings file changed "}; message.startsWith(prefix)) {
        gChangedPaths.append(message.sliced(prefix.size()));
    }
    gPreviousHandler(type, context, message);
}

// scoped, forwards every message so ignoreMessage keeps working
[[nodiscard]] auto recordWatcherEvents() {
    gChangedPaths.clear();
    gPreviousHandler = qInstallMessageHandler(recordChangedPaths);
    return qScopeGuard([] { qInstallMessageHandler(gPreviousHandler); });
}

// what QML reads, sorted since the set has no order
[[nodiscard]] QStringList disabledModsProperty(const vsmm::GameMngr &gameMngr) {
    QStringList ids = gameMngr.property("disabledMods").toStringList();
    ids.sort();
    return ids;
}
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

        QTest::ignoreMessage(QtInfoMsg, "No disabled mods");
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

        QTest::ignoreMessage(QtInfoMsg, "No disabled mods");
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

        QTest::ignoreMessage(QtInfoMsg, "No disabled mods");
        config.validate();

        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({dir.absoluteFilePath(u"mods1"_s)}));
    }

    void unreadableClientSettingsLeaveModsDirsEmpty_data() {
        QTest::addColumn<QByteArray>("contents");
        QTest::addColumn<bool>("writeTheFile");
        // each row pins why it gave up, so the diagnostics stay accurate
        QTest::addColumn<QByteArrayList>("expectedErrors");
        QTest::addColumn<QByteArray>("expectedInfo");
        QTest::newRow("file-missing") << QByteArray{} << false << QByteArrayList{"Client settings file does not exist"}
                                      << QByteArray{};
        QTest::newRow("not-json") << QByteArray{"not json at all"} << true
                                  << QByteArrayList{"Client settings file is not a valid JSON object"} << QByteArray{};
        QTest::newRow("json-array") << QByteArray{"[1, 2, 3]"} << true
                                    << QByteArrayList{"Client settings file is not a valid JSON object"}
                                    << QByteArray{};
        // both lists live in stringListSettings, so each one reports it missing
        QTest::newRow("stringListSettings-missing")
            << QJsonDocument{QJsonObject{{"stringSettings"_L1,
                                          QJsonObject{{"settingsVersion"_L1, SUPPORTED_SETTINGS_VERSION}}}}}
                   .toJson()
            << true
            << QByteArrayList{"Failed to locate stringListSettings in client settings file", "Invalid disabledMods",
                              "Invalid modPaths"}
            << QByteArray{};
        QTest::newRow("modPaths-not-an-array")
            << QJsonDocument{QJsonObject{
                                 {"stringSettings"_L1, QJsonObject{{"settingsVersion"_L1, SUPPORTED_SETTINGS_VERSION}}},
                                 {"stringListSettings"_L1,
                                  QJsonObject{{"modPaths"_L1, "/opt/mods"_L1}, {"disabledMods"_L1, QJsonArray{}}}}}}
                   .toJson()
            << true << QByteArrayList{"Invalid modPaths"} << QByteArray{"No disabled mods"};
    }

    void unreadableClientSettingsLeaveModsDirsEmpty() {
        QFETCH(QByteArray, contents);
        QFETCH(bool, writeTheFile);
        QFETCH(QByteArrayList, expectedErrors);
        QFETCH(QByteArray, expectedInfo);
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

        for (const auto &error : expectedErrors) {
            QTest::ignoreMessage(QtCriticalMsg, error.constData());
        }
        if (!expectedInfo.isEmpty()) {
            QTest::ignoreMessage(QtInfoMsg, expectedInfo.constData());
        }
        config.validate();

        QVERIFY(gameMngr.getModsDirs().isEmpty());
        // consumers still have to learn the scan produced nothing
        QCOMPARE(spy.count(), 1);
    }

    // inotify needs read access, so a file that cannot be read cannot be watched either
    void unreadableClientSettingsAreNotWatched() {
        SKIP_WITHOUT_FILE_PERMISSIONS();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(dir, clientSettings({dir.absoluteFilePath(u"mods1"_s)}, {u"carryon@1.0.0"_s})));
        const QString path = QDir::cleanPath(dir.filePath(CLIENT_SETTINGS_FILE));
        QVERIFY(QFile::setPermissions(path, QFileDevice::WriteOwner));

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Failed to watch client settings file %1"_s.arg(path)));
        QTest::ignoreMessage(QtCriticalMsg, qPrintable(u"Failed to open client settings file %1"_s.arg(path)));
        config.validate();

        QVERIFY(gameMngr.getModsDirs().isEmpty());
        QVERIFY(gameMngr.getDisabledMods().isEmpty());
        QCOMPARE(spy.count(), 1);
    }

    // unsupported settings file must not be parsed at all
    void unsupportedClientSettingsAreRejected_data() {
        QTest::addColumn<QJsonObject>("json");
        QTest::addColumn<QByteArray>("expectedError");
        QTest::newRow("unsupported-version")
            << clientSettings({u"/opt/vs/mods1"_s}, {}, "1.15"_L1)
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
        QTest::ignoreMessage(QtInfoMsg, "No disabled mods");
        config.validate();
        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({first.absoluteFilePath(u"mods1"_s)}));

        // another game install replaces the dirs, it does not append to them
        QTest::ignoreMessage(QtInfoMsg, "No disabled mods");
        config.setGameConfigDir(secondGameDir.path());

        QCOMPARE(spy.count(), 2);
        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({second.absoluteFilePath(u"mods2"_s)}));
    }

    void readsDisabledModsFromClientSettings() {
        LoadedGameMngr game;
        QSignalSpy spy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        QVERIFY(game.load({u"carryon@1.2.3"_s, u"betterruins@1.0.0"_s, u"primitivesurvival"_s}));

        QVERIFY(game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        QVERIFY(game.gameMngr.getDisabledMods().contains(u"betterruins"_s));
        // an entry without a version still names a mod
        QVERIFY(game.gameMngr.getDisabledMods().contains(u"primitivesurvival"_s));
        QVERIFY(!game.gameMngr.getDisabledMods().contains(u"expandedfoods"_s));
        // lookups are by id, never by the whole entry
        QVERIFY(!game.gameMngr.getDisabledMods().contains(u"carryon@1.2.3"_s));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(disabledModsProperty(game.gameMngr),
                 (QStringList{u"betterruins"_s, u"carryon"_s, u"primitivesurvival"_s}));
    }

    void skipsUnusableDisabledModEntries_data() {
        QTest::addColumn<QJsonValue>("entry");
        QTest::addColumn<QByteArray>("expectedWarning");
        QTest::newRow("null") << QJsonValue{QJsonValue::Null} << QByteArray{};
        QTest::newRow("empty-string") << QJsonValue{""_L1} << QByteArray{};
        QTest::newRow("not-a-string") << QJsonValue{42} << QByteArray{};
        QTest::newRow("no-id") << QJsonValue{"@1.0.0"_L1} << QByteArray{"Invalid disabled mod id @1.0.0"};
    }

    void skipsUnusableDisabledModEntries() {
        QFETCH(QJsonValue, entry);
        QFETCH(QByteArray, expectedWarning);
        LoadedGameMngr game;
        QSignalSpy spy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        if (!expectedWarning.isEmpty()) {
            QTest::ignoreMessage(QtWarningMsg, expectedWarning.constData());
        }
        QVERIFY(game.load({entry, u"carryon@1.2.3"_s}));

        QCOMPARE(spy.count(), 1);
        QVERIFY(!game.gameMngr.getDisabledMods().contains(QString{}));
    }

    void readDisabledModsAreLogged_data() {
        QTest::addColumn<QJsonArray>("disabledMods");
        QTest::addColumn<QString>("expectedInfo");
        QTest::newRow("none") << QJsonArray{} << u"^No disabled mods$"_s;
        QTest::newRow("only-unusable") << QJsonArray{QJsonValue::Null, ""_L1} << u"^No disabled mods$"_s;
        QTest::newRow("one") << QJsonArray{u"carryon@1.2.3"_s} << u"^Disabled mods \\(1\\): carryon$"_s;
        QTest::newRow("unusable-entry-skipped")
            << QJsonArray{QJsonValue::Null, u"carryon@1.2.3"_s} << u"^Disabled mods \\(1\\): carryon$"_s;
        // set order is not fixed
        QTest::newRow("two") << QJsonArray{u"carryon@1.2.3"_s, u"betterruins@1.0.0"_s}
                             << u"^Disabled mods \\(2\\): (carryon, betterruins|betterruins, carryon)$"_s;
    }

    void readDisabledModsAreLogged() {
        QFETCH(QJsonArray, disabledMods);
        QFETCH(QString, expectedInfo);
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(dir, clientSettings({dir.absoluteFilePath(u"mods1"_s)}, disabledMods)));

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);

        QTest::ignoreMessage(QtInfoMsg, QRegularExpression{expectedInfo});
        config.validate();
    }

    void disabledModsThatAreNotAListAreIgnored() {
        LoadedGameMngr game;
        QJsonObject settings = clientSettings(game.modPaths());
        settings["stringListSettings"_L1] =
            QJsonObject{{"modPaths"_L1, game.modPaths()}, {"disabledMods"_L1, "carryon@1.2.3"_L1}};

        QTest::ignoreMessage(QtCriticalMsg, "Invalid disabledMods");
        QVERIFY(game.load(settings));

        QVERIFY(!game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        // mod paths do not depend on it
        QCOMPARE(toPaths(game.gameMngr.getModsDirs()), QStringList{game.dir().absoluteFilePath(u"mods1"_s)});
    }

    void addingADisabledModWritesItToTheGameConfig() {
        LoadedGameMngr game;
        QJsonObject settings = clientSettings(game.modPaths(), {u"betterruins@1.0.0"_s});
        // a setting the manager knows nothing about must survive the rewrite
        settings["intSettings"_L1] = QJsonObject{{"guiScale"_L1, 2}};
        QVERIFY(game.load(settings));
        QSignalSpy spy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        QTest::ignoreMessage(QtInfoMsg, "Mod carryon disabled");
        game.gameMngr.addDisabledMod(u"carryon"_s, u"1.2.3"_s);

        QVERIFY(game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(disabledModsProperty(game.gameMngr), (QStringList{u"betterruins"_s, u"carryon"_s}));
        QJsonObject expected = clientSettings(game.modPaths(), {u"betterruins@1.0.0"_s, u"carryon@1.2.3"_s});
        expected["intSettings"_L1] = QJsonObject{{"guiScale"_L1, 2}};
        QCOMPARE(readClientSettings(game.dir()), expected);
    }

    // every rescan of a disabled mod adds its loaded version, so this must not pile up
    void addingAnEntryThatIsThereKeepsOneCopy() {
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.2.3"_s}));
        QSignalSpy spy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        game.gameMngr.addDisabledMod(u"carryon"_s, u"1.2.3"_s);

        QCOMPARE(disabledModsOnDisk(game.dir()), QJsonArray{u"carryon@1.2.3"_s});
        QCOMPARE(spy.count(), 0);
    }

    void aNewVersionOfADisabledModIsAddedNextToTheOldOne() {
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.0.0"_s}));
        QSignalSpy spy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        game.gameMngr.addDisabledMod(u"carryon"_s, u"1.2.3"_s);

        QCOMPARE(disabledModsOnDisk(game.dir()), (QJsonArray{u"carryon@1.0.0"_s, u"carryon@1.2.3"_s}));
        QVERIFY(game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        // it was disabled already
        QCOMPARE(spy.count(), 0);
    }

    // two loads queued together write before the watcher catches up
    void twoAddsInOnePassKeepBoth() {
        LoadedGameMngr game;
        QVERIFY(game.load(QJsonArray{}));

        QTest::ignoreMessage(QtInfoMsg, "Mod carryon disabled");
        game.gameMngr.addDisabledMod(u"carryon"_s, u"1.0.0"_s);
        QTest::ignoreMessage(QtInfoMsg, "Mod betterruins disabled");
        game.gameMngr.addDisabledMod(u"betterruins"_s, u"1.0.0"_s);

        QCOMPARE(disabledModsOnDisk(game.dir()), (QJsonArray{u"carryon@1.0.0"_s, u"betterruins@1.0.0"_s}));
    }

    void addingAModWithoutAnIdIsRejected() {
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.0.0"_s}));
        QSignalSpy spy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        QTest::ignoreMessage(QtWarningMsg, "Invalid disabled mod id");
        game.gameMngr.addDisabledMod(QString{}, u"1.2.3"_s);

        QCOMPARE(disabledModsOnDisk(game.dir()), QJsonArray{u"carryon@1.0.0"_s});
        QCOMPARE(spy.count(), 0);
    }

    void removingADisabledModDropsEveryVersionOfIt() {
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.0.0"_s, u"carryonplus@1.0.0"_s, u"carryon@1.2.3"_s, u"car@2.0.0"_s, u"c"_s}));
        QSignalSpy spy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        QTest::ignoreMessage(QtInfoMsg, "Mod carryon enabled");
        game.gameMngr.removeDisabledMod(u"carryon"_s);

        // ids sharing its prefix and entries shorter than it stay
        QCOMPARE(disabledModsOnDisk(game.dir()), (QJsonArray{u"carryonplus@1.0.0"_s, u"car@2.0.0"_s, u"c"_s}));
        QVERIFY(!game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        QVERIFY(game.gameMngr.getDisabledMods().contains(u"carryonplus"_s));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(disabledModsProperty(game.gameMngr), (QStringList{u"c"_s, u"car"_s, u"carryonplus"_s}));
    }

    void removeDisabledModIsRejected_data() {
        QTest::addColumn<QString>("modId");
        QTest::addColumn<QByteArray>("expectedWarning");
        QTest::newRow("no-id") << QString{} << QByteArray{"Invalid disabled mod id"};
        QTest::newRow("not-disabled") << u"expandedfoods"_s << QByteArray{"Mod expandedfoods is not disabled"};
    }

    void removeDisabledModIsRejected() {
        QFETCH(QString, modId);
        QFETCH(QByteArray, expectedWarning);
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.0.0"_s}));
        QSignalSpy spy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        QTest::ignoreMessage(QtWarningMsg, expectedWarning.constData());
        game.gameMngr.removeDisabledMod(modId);

        QCOMPARE(disabledModsOnDisk(game.dir()), QJsonArray{u"carryon@1.0.0"_s});
        QCOMPARE(spy.count(), 0);
    }

    void unreadableGameConfigIsNeverOverwritten_data() {
        QTest::addColumn<bool>("add");
        QTest::newRow("add") << true;
        QTest::newRow("remove") << false;
    }

    // writing from nothing would wipe every other game setting
    void unreadableGameConfigIsNeverOverwritten() {
        QFETCH(bool, add);
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(dir, QByteArray{"not json at all"}));

        vsmm::ConfigMock config;
        config.setGameConfigDir(gameDir.path());

        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QTest::ignoreMessage(QtCriticalMsg, "Client settings file is not a valid JSON object");
        config.validate();

        QTest::ignoreMessage(QtCriticalMsg, "Client settings are empty");
        if (add) {
            gameMngr.addDisabledMod(u"carryon"_s, u"1.2.3"_s);
        } else {
            gameMngr.removeDisabledMod(u"carryon"_s);
        }

        QCOMPARE(readFile(dir.absoluteFilePath(CLIENT_SETTINGS_FILE)), QByteArray{"not json at all"});
        QVERIFY(!gameMngr.getDisabledMods().contains(u"carryon"_s));
    }

    void gameConfigThatCannotBeWrittenChangesNothing_data() {
        QTest::addColumn<bool>("add");
        QTest::newRow("add") << true;
        QTest::newRow("remove") << false;
    }

    void gameConfigThatCannotBeWrittenChangesNothing() {
        QFETCH(bool, add);
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.0.0"_s}));
        QSignalSpy spy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};
        // a dir under the file's name makes the save fail to open
        QVERIFY(QFile::remove(game.settingsPath()));
        QVERIFY(game.dir().mkdir(CLIENT_SETTINGS_FILE));

        QTest::ignoreMessage(QtCriticalMsg, "Failed to open client settings file for writing");
        if (add) {
            game.gameMngr.addDisabledMod(u"betterruins"_s, u"1.0.0"_s);
        } else {
            game.gameMngr.removeDisabledMod(u"carryon"_s);
        }

        QVERIFY(!game.gameMngr.getDisabledMods().contains(u"betterruins"_s));
        QVERIFY(game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        QCOMPARE(spy.count(), 0);
    }

    // the game rewrites the file when the player toggles a mod in its own menu
    void outsideChangesToDisabledModsAreReadBack() {
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.0.0"_s}));
        QSignalSpy disabledSpy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};
        QSignalSpy scanSpy{&game.gameMngr, &vsmm::GameMngr::modsDirsChanged};

        QVERIFY(writeClientSettings(game.dir(), clientSettings(game.modPaths(), {u"betterruins@1.0.0"_s})));

        QTRY_VERIFY(game.gameMngr.getDisabledMods().contains(u"betterruins"_s));
        QVERIFY(!game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        QCOMPARE(disabledSpy.count(), 1);
        // same mod paths, no reason to rescan
        QCOMPARE(scanSpy.count(), 0);
    }

    void outsideChangesToModPathsTriggerARescan() {
        LoadedGameMngr game;
        QVERIFY(game.load(QJsonArray{}));
        QSignalSpy scanSpy{&game.gameMngr, &vsmm::GameMngr::modsDirsChanged};

        const QString mods2 = game.dir().absoluteFilePath(u"mods2"_s);
        QVERIFY(writeClientSettings(game.dir(), clientSettings({mods2})));

        QTRY_COMPARE(scanSpy.count(), 1);
        QCOMPARE(toPaths(game.gameMngr.getModsDirs()), QStringList{mods2});
    }

    // a rescan from its own write would reload every mod, which then writes again
    void ownWritesDoNotTriggerARescan() {
        LoadedGameMngr game;
        QVERIFY(game.load(QJsonArray{}));
        QSignalSpy scanSpy{&game.gameMngr, &vsmm::GameMngr::modsDirsChanged};
        const auto watching = recordWatcherEvents();

        QTest::ignoreMessage(QtInfoMsg, "Mod carryon disabled");
        game.gameMngr.addDisabledMod(u"carryon"_s, u"1.0.0"_s);
        QTRY_VERIFY(gChangedPaths.contains(game.settingsPath()));

        // two saves can land in one clock tick, push the first one back so the second differs
        gChangedPaths.clear();
        QVERIFY(backdate(game.settingsPath()));
        QTRY_VERIFY(gChangedPaths.contains(game.settingsPath()));

        // the save replaced the file, so this only shows up if it is watched again
        gChangedPaths.clear();
        QTest::ignoreMessage(QtInfoMsg, "Mod betterruins disabled");
        game.gameMngr.addDisabledMod(u"betterruins"_s, u"1.0.0"_s);
        QTRY_VERIFY(gChangedPaths.contains(game.settingsPath()));

        QCOMPARE(scanSpy.count(), 0);
        QVERIFY(game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        QVERIFY(game.gameMngr.getDisabledMods().contains(u"betterruins"_s));
        QCOMPARE(disabledModsOnDisk(game.dir()), (QJsonArray{u"carryon@1.0.0"_s, u"betterruins@1.0.0"_s}));
    }

    // the game can be caught mid-write, the next change brings the file back
    void aBrokenRewriteKeepsTheLastGoodState() {
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.0.0"_s}));
        QSignalSpy disabledSpy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};
        QSignalSpy scanSpy{&game.gameMngr, &vsmm::GameMngr::modsDirsChanged};
        const auto watching = recordWatcherEvents();

        QTest::ignoreMessage(QtCriticalMsg, "Client settings file is not a valid JSON object");
        QVERIFY(writeClientSettings(game.dir(), QByteArray{"{\"stringSettings\": {"}));
        QTRY_VERIFY(gChangedPaths.contains(game.settingsPath()));

        QVERIFY(game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        QCOMPARE(toPaths(game.gameMngr.getModsDirs()), QStringList{game.dir().absoluteFilePath(u"mods1"_s)});
        QCOMPARE(disabledSpy.count(), 0);
        QCOMPARE(scanSpy.count(), 0);
    }

    void switchingGameConfigDirMovesTheWatch() {
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.0.0"_s}));
        QTemporaryDir otherGameDir;
        QVERIFY(otherGameDir.isValid());
        const QDir other{otherGameDir.path()};
        const QJsonArray otherModPaths{other.absoluteFilePath(u"mods2"_s)};
        const QString otherPath = QDir::cleanPath(other.filePath(CLIENT_SETTINGS_FILE));
        QVERIFY(writeClientSettings(other, clientSettings(otherModPaths, {u"betterruins@1.0.0"_s})));
        QVERIFY(backdate(otherPath));
        QSignalSpy disabledSpy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        QTest::ignoreMessage(QtInfoMsg, "Disabled mods (1): betterruins");
        game.config.setGameConfigDir(otherGameDir.path());

        QVERIFY(game.gameMngr.getDisabledMods().contains(u"betterruins"_s));
        QVERIFY(!game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        QCOMPARE(disabledSpy.count(), 1);

        const auto watching = recordWatcherEvents();
        // old file first, its event cannot come after the new one
        QVERIFY(writeClientSettings(game.dir(), clientSettings(game.modPaths())));
        QVERIFY(writeClientSettings(other, clientSettings(otherModPaths)));

        QTRY_VERIFY(gChangedPaths.contains(otherPath));
        QVERIFY(!gChangedPaths.contains(game.settingsPath()));
        QVERIFY(!game.gameMngr.getDisabledMods().contains(u"betterruins"_s));
    }

    // a failed switch must not leave the old file's state around to be written into the new dir
    void switchingToAnUnreadableGameConfigForgetsDisabledMods() {
        LoadedGameMngr game;
        QVERIFY(game.load({u"carryon@1.0.0"_s}));
        QTemporaryDir emptyGameDir;
        QVERIFY(emptyGameDir.isValid());
        QSignalSpy disabledSpy{&game.gameMngr, &vsmm::GameMngr::disabledModsChanged};

        QTest::ignoreMessage(QtCriticalMsg, "Client settings file does not exist");
        game.config.setGameConfigDir(emptyGameDir.path());

        QVERIFY(!game.gameMngr.getDisabledMods().contains(u"carryon"_s));
        QCOMPARE(disabledSpy.count(), 1);

        QTest::ignoreMessage(QtCriticalMsg, "Client settings are empty");
        game.gameMngr.addDisabledMod(u"betterruins"_s, u"1.0.0"_s);

        QVERIFY(!QFile::exists(QDir{emptyGameDir.path()}.absoluteFilePath(CLIENT_SETTINGS_FILE)));
        QCOMPARE(disabledModsOnDisk(game.dir()), QJsonArray{u"carryon@1.0.0"_s});
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
        QTest::ignoreMessage(QtInfoMsg, "No disabled mods");
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
        QTest::ignoreMessage(QtInfoMsg, "No disabled mods");
        config.validate();

        QSignalSpy scanSpy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};
        bool versionKnownAtScan = false;
        connect(&gameMngr, &vsmm::GameMngr::modsDirsChanged, this,
                [&] { versionKnownAtScan = gameMngr.getGameVersion().has_value(); });

        const QString exe = writeFakeGameExe(second, u"1.22.5"_s);
        QVERIFY(!exe.isEmpty());
        // one Settings save changing both paths at once
        QTest::ignoreMessage(QtInfoMsg, "No disabled mods");
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
