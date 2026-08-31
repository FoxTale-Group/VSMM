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

#include <Config.hpp>

#include <QJsonArray>
#include <QLoggingCategory>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class ConfigIntegrationTest : public QObject {
    Q_OBJECT

    [[nodiscard]] static QString configDir() {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    [[nodiscard]] static QString configFilePath() {
        return configDir() + QDir::separator() + vsmm::IConfig::CONFIG_FILE_NAME;
    }

    static void createConfigFile(const QByteArray &contents) {
        QVERIFY(QDir().mkpath(configDir()));
        QFile file{configFilePath()};
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
        QCOMPARE(file.write(contents), contents.size());
    }

    [[nodiscard]] static QJsonObject readConfigFile() {
        QFile file{configFilePath()};
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        return QJsonDocument::fromJson(file.readAll()).object();
    }

  private slots:
    void initTestCase() {
        QLoggingCategory::setFilterRules(u"*.debug=false\n*.info=false"_s);
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(configDir().contains("qttest"_L1), qPrintable(configDir()));
        if (QFile::exists(configFilePath())) {
            QVERIFY(QFile::remove(configFilePath()));
        }
    }

    void cleanupTestCase() {
        QFile::remove(configFilePath());
        QStandardPaths::setTestModeEnabled(false);
    }

    void missingFileYieldsDefaultConfig() {
        QVERIFY(!QFile::exists(configFilePath()));

        QTest::ignoreMessage(QtWarningMsg, "Could not open config file");
        vsmm::Config config;

        QVERIFY(config.paths().gameConfig.isEmpty());
        QVERIFY(config.paths().gameExe.isEmpty());
        QVERIFY(config.getFavorites().isEmpty());
        QCOMPARE(config.general().deleteOldModVersion, vsmm::GeneralSettings::DEFAULT_VALUE_DELETE_OLD_MOD_VERSION);
        QCOMPARE(config.general().includeModPrerelease, vsmm::GeneralSettings::DEFAULT_VALUE_INCLUDE_MOD_PRERELEASE);
        QCOMPARE(config.appearance().theme, vsmm::AppearanceSettings::DEFAULT_VALUE_THEME);
        QCOMPARE(config.appearance().accentIndex, vsmm::AppearanceSettings::DEFAULT_VALUE_ACCENT_IDX);
    }

    void readsExistingFile() {
        createConfigFile(R"({
            "general": {"deleteOldModVersion": true, "includeModPrerelease": false},
            "paths": {"gameConfig": "/opt/vs-config"},
            "appearance": {"theme": "dark"},
            "favorites": ["carryon", "petai"]
        })");

        vsmm::Config config;

        QCOMPARE(config.paths().gameConfig, u"/opt/vs-config"_s);
        QCOMPARE(config.general().deleteOldModVersion, true);
        QCOMPARE(config.general().includeModPrerelease, false);
        QCOMPARE(vsmm::themeToString(config.appearance().theme), u"dark"_s);
        QCOMPARE(config.getFavorites(), QStringList({u"carryon"_s, u"petai"_s}));
    }

    void nonObjectJsonYieldsDefaultConfig() {
        createConfigFile("[1, 2, 3]");

        QTest::ignoreMessage(QtWarningMsg, "Config file is not a valid JSON object");
        vsmm::Config config;

        QVERIFY(config.paths().gameConfig.isEmpty());
        QVERIFY(config.paths().gameExe.isEmpty());
        QVERIFY(config.getFavorites().isEmpty());
        QCOMPARE(config.general().deleteOldModVersion, vsmm::GeneralSettings::DEFAULT_VALUE_DELETE_OLD_MOD_VERSION);
        QCOMPARE(config.general().includeModPrerelease, vsmm::GeneralSettings::DEFAULT_VALUE_INCLUDE_MOD_PRERELEASE);
        QCOMPARE(config.appearance().theme, vsmm::AppearanceSettings::DEFAULT_VALUE_THEME);
        QCOMPARE(config.appearance().accentIndex, vsmm::AppearanceSettings::DEFAULT_VALUE_ACCENT_IDX);
    }

    void unusableConfigDirIsReported() {
        QFile::remove(configFilePath());
        if (QDir dir{configDir()}; dir.exists()) {
            QVERIFY(dir.removeRecursively());
        }
        QVERIFY(QDir{}.mkpath(QFileInfo{configDir()}.absolutePath()));
        const auto restoreDir = qScopeGuard([] { QFile::remove(configDir()); });
        QFile blocker{configDir()};
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();

        QTest::ignoreMessage(QtWarningMsg, "Could not create config directory");
        QTest::ignoreMessage(QtWarningMsg, "Could not open config file");
        QTest::ignoreMessage(QtWarningMsg, "Failed to open config file for writing");
        {
            vsmm::Config config;
            QVERIFY(config.paths().gameConfig.isEmpty());
            QCOMPARE(config.appearance().accentIndex, vsmm::AppearanceSettings::DEFAULT_VALUE_ACCENT_IDX);
        }
        QVERIFY(!QFile::exists(configFilePath()));
    }

    void favoritesRoundTripThroughDisk() {
        createConfigFile("{}");
        {
            vsmm::Config config;
            config.setFavorites({u"carryon"_s, u"petai"_s});
            QCOMPARE(readConfigFile().value("favorites"_L1).toArray().size(), 2);
        }
        vsmm::Config reloaded;
        QCOMPARE(reloaded.getFavorites(), QStringList({u"carryon"_s, u"petai"_s}));
    }

    void destructorPersistsConfig() {
        createConfigFile("{}");
        vsmm::appearance::Theme theme = vsmm::appearance::Theme::Light;
        {
            vsmm::Config config;
            QVERIFY(config.setProperty("appearance", QVariant::fromValue(vsmm::AppearanceSettings{.theme = theme})));
            QFile::remove(configFilePath());
        }
        QCOMPARE(readConfigFile()
                     .value(vsmm::AppearanceSettings::KEY_NAME)
                     .toObject()
                     .value(vsmm::AppearanceSettings::THEME_KEY)
                     .toString(),
                 vsmm::themeToString(theme));
    }

    void sectionPropertiesNotifyOnlyOnChange_data() {
        QTest::addColumn<QByteArray>("property");
        QTest::addColumn<QByteArray>("signalName");
        QTest::addColumn<QVariant>("value");
        QTest::addColumn<QJsonObject>("expectedJson");

        constexpr vsmm::GeneralSettings general{.deleteOldModVersion = false, .includeModPrerelease = true};
        const vsmm::PathSettings paths{.gameConfig = u"/opt/vs-config"_s, .gameExe = u"/opt/vs/game"_s};
        constexpr vsmm::AppearanceSettings appearance{.theme = vsmm::appearance::Theme::Light, .accentIndex = 3};

        QTest::newRow("general") << QByteArray{"general"} << QByteArray{SIGNAL(generalChanged())}
                                 << QVariant::fromValue(general) << QJsonObject::fromVariantHash(general.toHash());
        QTest::newRow("paths") << QByteArray{"paths"} << QByteArray{SIGNAL(pathsChanged())}
                               << QVariant::fromValue(paths) << QJsonObject::fromVariantHash(paths.toHash());
        QTest::newRow("appearance") << QByteArray{"appearance"} << QByteArray{SIGNAL(appearanceChanged())}
                                    << QVariant::fromValue(appearance)
                                    << QJsonObject::fromVariantHash(appearance.toHash());
    }

    void sectionPropertiesNotifyOnlyOnChange() {
        QFETCH(QByteArray, property);
        QFETCH(QByteArray, signalName);
        QFETCH(QVariant, value);
        QFETCH(QJsonObject, expectedJson);

        createConfigFile("{}");

        vsmm::Config config;
        QSignalSpy spy{&config, signalName.constData()};
        QVERIFY(spy.isValid());

        QVERIFY(config.setProperty(property.constData(), value));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(config.property(property.constData()), value);

        config.saveToFile();
        QCOMPARE(readConfigFile().value(QString::fromLatin1(property)).toObject(), expectedJson);

        // same value must not re-notify or QML rebinds for nothing
        QVERIFY(config.setProperty(property.constData(), value));
        QCOMPARE(spy.count(), 1);
    }

    void gameConfigPathChangedTracksOnlyGameConfig() {
        createConfigFile("{}");
        vsmm::Config config;
        QSignalSpy pathsSpy{&config, &vsmm::Config::pathsChanged};
        QSignalSpy gameConfigSpy{&config, &vsmm::Config::gameConfigPathChanged};

        QVERIFY(config.setProperty("paths", QVariant::fromValue(vsmm::PathSettings::fromHash(QVariantHash{
                                                {vsmm::PathSettings::GAME_CONFIG_KEY, u"/opt/vs-config"_s}}))));
        QCOMPARE(pathsSpy.count(), 1);
        QCOMPARE(gameConfigSpy.count(), 1);

        // another key changed, so paths changed but the game config dir did not
        QVERIFY(config.setProperty("paths", QVariant::fromValue(vsmm::PathSettings::fromHash(
                                                QVariantHash{{vsmm::PathSettings::GAME_CONFIG_KEY, u"/opt/vs-config"_s},
                                                             {vsmm::PathSettings::GAME_EXE_KEY, u"/opt/mods"_s}}))));
        QCOMPARE(pathsSpy.count(), 2);
        QCOMPARE(gameConfigSpy.count(), 1);

        QVERIFY(config.setProperty("paths", QVariant::fromValue(vsmm::PathSettings::fromHash(QVariantHash{
                                                {vsmm::PathSettings::GAME_CONFIG_KEY, u"/opt/other"_s}}))));
        QCOMPARE(pathsSpy.count(), 3);
        QCOMPARE(gameConfigSpy.count(), 2);
    }

    void validateRejectsBadConfig_data() {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QByteArray>("expectedError");
        const QByteArray invalidConfig{"VSMM config is not a valid"};
        const QByteArray badGamePath{"Config game path does not exist"};
        QTest::newRow("empty-file") << QByteArray{"{}"} << invalidConfig;
        QTest::newRow("general-missing") << QByteArray{R"({"paths": {"gameConfig": "."}})"} << invalidConfig;
        QTest::newRow("general-not-an-object")
            << QByteArray{R"({"general": "yes", "paths": {"gameConfig": "."}})"} << invalidConfig;
        QTest::newRow("game-config-path-empty")
            << QByteArray{R"({"general": {}, "paths": {"gameConfig": ""}})"} << badGamePath;
        QTest::newRow("game-config-path-missing") << QByteArray{R"({"general": {}, "paths": {}})"} << badGamePath;
        QTest::newRow("game-config-dir-absent")
            << QByteArray{R"({"general": {}, "paths": {"gameConfig": "/nope/does/not/exist"}})"} << badGamePath;
    }

    void validateConfig() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        createConfigFile(QJsonDocument{
            QJsonObject{
                {vsmm::GeneralSettings::KEY_NAME,
                 QJsonObject{{vsmm::GeneralSettings::DELETE_OLD_MOD_VERSION_KEY, true}}},
                {vsmm::AppearanceSettings::KEY_NAME, QJsonObject{{vsmm::AppearanceSettings::THEME_KEY,
                                                                  vsmm::themeToString(vsmm::appearance::Theme::Dark)}}},
                {vsmm::PathSettings::KEY_NAME, QJsonObject{{vsmm::PathSettings::GAME_CONFIG_KEY, gameDir.path()}}},
            }}.toJson());

        vsmm::Config config;
        QSignalSpy generalSpy{&config, &vsmm::Config::generalChanged};
        QSignalSpy pathsSpy{&config, &vsmm::Config::pathsChanged};
        QSignalSpy appearanceSpy{&config, &vsmm::Config::appearanceChanged};
        QSignalSpy gameConfigSpy{&config, &vsmm::Config::gameConfigPathChanged};

        config.validate();

        QCOMPARE(generalSpy.count(), 1);
        QCOMPARE(pathsSpy.count(), 1);
        QCOMPARE(appearanceSpy.count(), 1);
        QCOMPARE(gameConfigSpy.count(), 1);
    }
};

QTEST_GUILESS_MAIN(ConfigIntegrationTest)
#include "ConfigIntegrationTest.moc"
