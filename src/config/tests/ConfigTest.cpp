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
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class ConfigTest : public QObject {
    Q_OBJECT

    static constexpr QLatin1StringView GAME_CONFIG_KEY{"gameConfig"};

    [[nodiscard]] static QString configDir() {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    [[nodiscard]] static QString configFilePath() { return configDir() + QDir::separator() + "config.json"_L1; }

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
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(configDir().contains("qttest"_L1), qPrintable(configDir()));
    }

    void cleanupTestCase() {
        QFile::remove(configFilePath());
        QStandardPaths::setTestModeEnabled(false);
    }

    void missingFileYieldsEmptyConfig() {
        QVERIFY(!QFile::exists(configFilePath()));

        vsmm::Config config;

        QVERIFY(config.getPath(GAME_CONFIG_KEY).isEmpty());
        QVERIFY(config.getFavorites().isEmpty());
        QCOMPARE(config.getGeneral<bool>(vsmm::Config::DELETE_OLD_VERSION_JSON_KEY), false);
        QVERIFY(config.property("general").toHash().isEmpty());
    }

    void readsExistingFile() {
        createConfigFile(R"({
            "general": {"deleteOldModVersion": true, "includeModPrerelease": false},
            "paths": {"gameConfig": "/opt/vs-config", "mods": "/opt/mods"},
            "appearance": {"theme": "dark"},
            "favorites": ["carryon", "petai"]
        })");

        vsmm::Config config;

        QCOMPARE(config.getPath(GAME_CONFIG_KEY), u"/opt/vs-config"_s);
        QCOMPARE(config.getGeneral<bool>(vsmm::Config::DELETE_OLD_VERSION_JSON_KEY), true);
        QCOMPARE(config.getGeneral<bool>(vsmm::Config::INCLUDE_MOD_PRERELEASE_JSON_KEY), false);
        QCOMPARE(config.getAppearance<QString>("theme"_L1), u"dark"_s);
        QCOMPARE(config.getFavorites(), QStringList({u"carryon"_s, u"petai"_s}));
        QCOMPARE(config.property("paths").toHash().value("mods"_L1).toString(), u"/opt/mods"_s);
    }

    void nonObjectJsonYieldsEmptyConfig() {
        createConfigFile("[1, 2, 3]");

        vsmm::Config config;

        QVERIFY(config.property("general").toHash().isEmpty());
        QVERIFY(config.getPath(GAME_CONFIG_KEY).isEmpty());
    }

    void favoritesRoundTripThroughDisk() {
        {
            vsmm::Config config;
            config.setFavorites({u"carryon"_s, u"petai"_s});
            QCOMPARE(readConfigFile().value("favorites"_L1).toArray().size(), 2);
        }
        vsmm::Config reloaded;
        QCOMPARE(reloaded.getFavorites(), QStringList({u"carryon"_s, u"petai"_s}));
    }

    void destructorPersistsConfig() {
        {
            vsmm::Config config;
            QVERIFY(config.setProperty("appearance", QVariantHash{{u"theme"_s, u"dark"_s}}));
            QFile::remove(configFilePath());
        }
        QCOMPARE(readConfigFile().value("appearance"_L1).toObject().value("theme"_L1).toString(), u"dark"_s);
    }

    void sectionPropertiesNotifyOnlyOnChange_data() {
        QTest::addColumn<QByteArray>("property");
        QTest::addColumn<QByteArray>("signalName");
        QTest::newRow("general") << QByteArray{"general"} << QByteArray{SIGNAL(generalChanged())};
        QTest::newRow("paths") << QByteArray{"paths"} << QByteArray{SIGNAL(pathsChanged())};
        QTest::newRow("appearance") << QByteArray{"appearance"} << QByteArray{SIGNAL(appearanceChanged())};
    }

    void sectionPropertiesNotifyOnlyOnChange() {
        QFETCH(QByteArray, property);
        QFETCH(QByteArray, signalName);

        vsmm::Config config;
        QSignalSpy spy{&config, signalName.constData()};
        QVERIFY(spy.isValid());

        const QVariantHash value{{u"key"_s, u"value"_s}};
        QVERIFY(config.setProperty(property.constData(), value));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(config.property(property.constData()).toHash(), value);
        QCOMPARE(readConfigFile().value(QString::fromLatin1(property)).toObject().value("key"_L1).toString(),
                 u"value"_s);

        // Same value again must not re-notify or QML rebinds for nothing.
        QVERIFY(config.setProperty(property.constData(), value));
        QCOMPARE(spy.count(), 1);
    }

    void gameConfigPathChangedTracksOnlyGameConfig() {
        vsmm::Config config;
        QSignalSpy pathsSpy{&config, &vsmm::Config::pathsChanged};
        QSignalSpy gameConfigSpy{&config, &vsmm::Config::gameConfigPathChanged};

        QVERIFY(config.setProperty("paths", QVariantHash{{u"gameConfig"_s, u"/opt/vs-config"_s}}));
        QCOMPARE(pathsSpy.count(), 1);
        QCOMPARE(gameConfigSpy.count(), 1);

        // A different key moves: paths changed, but the game config dir did not.
        QVERIFY(config.setProperty("paths",
                                   QVariantHash{{u"gameConfig"_s, u"/opt/vs-config"_s}, {u"mods"_s, u"/opt/mods"_s}}));
        QCOMPARE(pathsSpy.count(), 2);
        QCOMPARE(gameConfigSpy.count(), 1);

        QVERIFY(config.setProperty("paths", QVariantHash{{u"gameConfig"_s, u"/opt/other"_s}}));
        QCOMPARE(pathsSpy.count(), 3);
        QCOMPARE(gameConfigSpy.count(), 2);
    }

    void validateRejectsBadConfig_data() {
        QTest::addColumn<QByteArray>("json");
        QTest::newRow("empty file") << QByteArray{"{}"};
        QTest::newRow("general missing") << QByteArray{R"({"paths": {"gameConfig": "."}})"};
        QTest::newRow("general not an object") << QByteArray{R"({"general": "yes", "paths": {"gameConfig": "."}})"};
        QTest::newRow("game config path empty") << QByteArray{R"({"general": {}, "paths": {"gameConfig": ""}})"};
        QTest::newRow("game config path missing") << QByteArray{R"({"general": {}, "paths": {}})"};
        QTest::newRow("game config dir absent")
            << QByteArray{R"({"general": {}, "paths": {"gameConfig": "/nope/does/not/exist"}})"};
    }

    void validateRejectsBadConfig() {
        QFETCH(QByteArray, json);
        createConfigFile(json);

        vsmm::Config config;
        QSignalSpy generalSpy{&config, &vsmm::Config::generalChanged};
        QSignalSpy pathsSpy{&config, &vsmm::Config::pathsChanged};
        QSignalSpy appearanceSpy{&config, &vsmm::Config::appearanceChanged};
        QSignalSpy gameConfigSpy{&config, &vsmm::Config::gameConfigPathChanged};

        config.validate();

        QCOMPARE(generalSpy.count(), 0);
        QCOMPARE(pathsSpy.count(), 0);
        QCOMPARE(appearanceSpy.count(), 0);
        QCOMPARE(gameConfigSpy.count(), 0);
    }

    void validateAcceptsGoodConfig() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        createConfigFile(QJsonDocument{
            QJsonObject{
                {"general"_L1, QJsonObject{{"deleteOldModVersion"_L1, true}}},
                {"appearance"_L1, QJsonObject{{"theme"_L1, "dark"_L1}}},
                {"paths"_L1, QJsonObject{{"gameConfig"_L1, gameDir.path()}}},
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

QTEST_GUILESS_MAIN(ConfigTest)
#include "ConfigTest.moc"
