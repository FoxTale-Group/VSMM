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

#include <Config.hpp>
#include <GameMngr.hpp>

#include <QLoggingCategory>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

class GameMngrIntegrationTest : public QObject {
    Q_OBJECT

    [[nodiscard]] static QString configDir() {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    [[nodiscard]] static QString configFilePath() { return configDir() + QDir::separator() + "config.json"_L1; }

    // config.json as the app would find it on disk, gameExe is optional
    [[nodiscard]] static bool writeConfig(const QString &gameConfigDir, const QString &gameExe = {}) {
        if (!QDir().mkpath(configDir())) {
            return false;
        }
        QJsonObject paths{{vsmm::PathSettings::GAME_CONFIG_KEY, gameConfigDir}};
        if (!gameExe.isEmpty()) {
            paths[vsmm::PathSettings::GAME_EXE_KEY] = gameExe;
        }
        return writeFile(configFilePath(),
                         QJsonDocument{QJsonObject{{vsmm::GeneralSettings::KEY_NAME, QJsonObject{}},
                                                   {vsmm::AppearanceSettings::KEY_NAME, QJsonObject{}},
                                                   {vsmm::PathSettings::KEY_NAME, paths}}}
                             .toJson());
    }

    static void ignoreEmptyGameExe() { QTest::ignoreMessage(QtCriticalMsg, "config paths.gameExe value is empty"); }

  private slots:
    void initTestCase() {
        QLoggingCategory::setFilterRules(u"*=false\ngamemngr=true\ngamemngr.debug=false"_s);
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(configDir().contains("qttest"_L1), qPrintable(configDir()));
    }

    void cleanupTestCase() {
        QFile::remove(configFilePath());
        QStandardPaths::setTestModeEnabled(false);
    }

    // validate() starts the whole pipeline and GameMngr hangs the mods scan off gameConfigPathChanged
    void validateDrivesTheModsDirScan() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(
            dir, clientSettings({u"Mods"_s, dir.absoluteFilePath(u"mods1"_s), dir.absoluteFilePath(u"mods2"_s)})));
        QVERIFY(writeConfig(gameDir.path()));

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::modsDirsChanged};

        config.validate();

        QCOMPARE(spy.count(), 1);
        QCOMPARE(toPaths(gameMngr.getModsDirs()),
                 QStringList({dir.absoluteFilePath(u"mods1"_s), dir.absoluteFilePath(u"mods2"_s)}));
    }

    // a new gameExe saved through the paths property must reach GameMngr and re-read the version
    void settingGameExeThroughThePathsPropertyReReadsTheVersion() {
        SKIP_WITHOUT_POSIX_SHELL();
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        const QDir dir{gameDir.path()};
        QVERIFY(writeClientSettings(dir, clientSettings({})));
        QVERIFY(writeConfig(gameDir.path()));

        vsmm::Config config;
        vsmm::GameMngr gameMngr;
        QSignalSpy spy{&gameMngr, &vsmm::GameMngr::gameVersionChanged};
        ignoreEmptyGameExe();
        gameMngr.setConfig(&config);
        QVERIFY(!gameMngr.getGameVersion());
        QCOMPARE(spy.count(), 0);

        const QString exe = writeFakeGameExe(dir, u"1.22.5"_s);
        QVERIFY(!exe.isEmpty());
        QVERIFY(config.setProperty("paths", QVariant::fromValue(vsmm::PathSettings::fromHash(
                                                QVariantHash{{vsmm::PathSettings::GAME_CONFIG_KEY, gameDir.path()},
                                                             {vsmm::PathSettings::GAME_EXE_KEY, exe}}))));

        QTRY_VERIFY(gameMngr.getGameVersion());
        QCOMPARE(QString::fromStdString(gameMngr.getGameVersion()->to_string()), u"1.22.5"_s);
        QCOMPARE(gameMngr.property("gameVersion").toString(), u"1.22.5"_s);
        QCOMPARE(spy.count(), 1);
    }

    // why this suite exists: setPaths emits gameExePathChanged before gameConfigPathChanged, no mock catches a swap
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
        QVERIFY(writeConfig(firstGameDir.path()));

        vsmm::Config config;
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
        QVERIFY(config.setProperty("paths", QVariant::fromValue(vsmm::PathSettings::fromHash(QVariantHash{
                                                {vsmm::PathSettings::GAME_CONFIG_KEY, secondGameDir.path()},
                                                {vsmm::PathSettings::GAME_EXE_KEY, exe}}))));

        // version read still in flight, so no scan announced yet
        QCOMPARE(scanSpy.count(), 0);

        QTRY_COMPARE(scanSpy.count(), 1);
        QVERIFY2(versionKnownAtScan, "scan was announced while the game version was still unknown");
        QCOMPARE(toPaths(gameMngr.getModsDirs()), QStringList({second.absoluteFilePath(u"mods2"_s)}));
    }
};

QTEST_GUILESS_MAIN(GameMngrIntegrationTest)
#include "GameMngrIntegrationTest.moc"
