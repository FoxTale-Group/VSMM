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

#include <QLoggingCategory>
#include <QScopeGuard>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

#if defined(Q_OS_WIN) || defined(Q_OS_DARWIN)
#ifdef _MSC_VER
#pragma warning(disable : 4702)
#endif
#define SKIP_WITHOUT_SETTINGS_FILE() QSKIP("Settings are not a plain file on Windows and macOS")
#else
#define SKIP_WITHOUT_SETTINGS_FILE() ((void)0)
#endif

class ConfigIntegrationTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase() {
        QLoggingCategory::setFilterRules(u"*.debug=false\n*.info=false"_s);
        // QSettings needs an org name, a test one keeps the real app settings untouched
        QCoreApplication::setOrganizationName(u"vsmm-tests"_s);
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(QSettings{}.fileName().contains("vsmm-tests"_L1), qPrintable(QSettings{}.fileName()));
    }

    void init() { QSettings{}.clear(); }

    void cleanupTestCase() {
        QSettings{}.clear();
        QStandardPaths::setTestModeEnabled(false);
    }

    void emptySettingsYieldDefaultConfig() {
        vsmm::Config config;

        QVERIFY(config.paths().gameConfig.isEmpty());
        QVERIFY(config.paths().gameExe.isEmpty());
        QVERIFY(config.getFavorites().isEmpty());
        QCOMPARE(config.general().deleteOldModVersion, vsmm::GeneralSettings::DEFAULT_VALUE_DELETE_OLD_MOD_VERSION);
        QCOMPARE(config.general().includeModPrerelease, vsmm::GeneralSettings::DEFAULT_VALUE_INCLUDE_MOD_PRERELEASE);
        QCOMPARE(config.appearance().theme, vsmm::AppearanceSettings::DEFAULT_VALUE_THEME);
        QCOMPARE(config.appearance().accentIndex, vsmm::AppearanceSettings::DEFAULT_VALUE_ACCENT_IDX);
    }

    void readsSavedSettings() {
        {
            QSettings settings;
            settings.setValue(u"deleteOldModVersion"_s, false);
            settings.setValue(u"includeModPrerelease"_s, true);
            settings.setValue(u"Paths/gameConfig"_s, u"/opt/vs-config"_s);
            settings.setValue(u"Paths/gameExe"_s, u"/opt/vs/game"_s);
            settings.setValue(u"Appearance/theme"_s, u"light"_s);
            settings.setValue(u"Appearance/accentIndex"_s, 2);
            settings.setValue(u"favorites"_s, QStringList{u"carryon"_s, u"petai"_s});
        }

        vsmm::Config config;

        QCOMPARE(config.general().deleteOldModVersion, false);
        QCOMPARE(config.general().includeModPrerelease, true);
        QCOMPARE(config.paths().gameConfig, u"/opt/vs-config"_s);
        QCOMPARE(config.paths().gameExe, u"/opt/vs/game"_s);
        QCOMPARE(config.appearance().theme, vsmm::appearance::Theme::Light);
        QCOMPARE(config.appearance().accentIndex, 2);
        QCOMPARE(config.getFavorites(), QStringList({u"carryon"_s, u"petai"_s}));
    }

    void unwritableSettingsAreReported() {
        SKIP_WITHOUT_SETTINGS_FILE();
        // a dir where the settings file should be, so it can be neither read nor written
        const QString path = QSettings{}.fileName();
        QFile::remove(path);
        QVERIFY(QDir{}.mkpath(path));
        const auto removeDir = qScopeGuard([&path] { QDir{path}.removeRecursively(); });

        QTest::ignoreMessage(QtWarningMsg, "Failed to write config file: AccessError");
        {
            vsmm::Config config;
            QVERIFY(config.paths().gameConfig.isEmpty());
            QCOMPARE(config.appearance().accentIndex, vsmm::AppearanceSettings::DEFAULT_VALUE_ACCENT_IDX);
        }
        QVERIFY(QFileInfo{path}.isDir());
    }

    void favoritesRoundTripThroughSettings() {
        {
            vsmm::Config config;
            config.setFavorites({u"carryon"_s, u"petai"_s});
            QCOMPARE(QSettings{}.value(u"favorites"_s).toStringList(), QStringList({u"carryon"_s, u"petai"_s}));
        }
        vsmm::Config reloaded;
        QCOMPARE(reloaded.getFavorites(), QStringList({u"carryon"_s, u"petai"_s}));

        // no favorites left, so the key goes away
        reloaded.setFavorites({});
        QVERIFY(!QSettings{}.contains(u"favorites"_s));
    }

    void destructorPersistsConfig() {
        {
            vsmm::Config config;
            QVERIFY(config.setProperty(
                "appearance", QVariant::fromValue(vsmm::AppearanceSettings{.theme = vsmm::appearance::Theme::Light})));
            QVERIFY(!QSettings{}.contains(u"Appearance/theme"_s));
        }
        QCOMPARE(QSettings{}.value(u"Appearance/theme"_s).toString(), u"light"_s);
    }

    void sectionPropertiesNotifyOnlyOnChange_data() {
        QTest::addColumn<QByteArray>("property");
        QTest::addColumn<QByteArray>("signalName");
        QTest::addColumn<QVariant>("value");
        QTest::addColumn<QString>("group");
        QTest::addColumn<QVariantMap>("expectedSaved");

        constexpr vsmm::GeneralSettings general{.deleteOldModVersion = false, .includeModPrerelease = true};
        const vsmm::PathSettings paths{.gameConfig = u"/opt/vs-config"_s, .gameExe = u"/opt/vs/game"_s};
        constexpr vsmm::AppearanceSettings appearance{.theme = vsmm::appearance::Theme::Light, .accentIndex = 3};

        // general has no group, its keys land in the default section
        QTest::newRow("general") << QByteArray{"general"} << QByteArray{SIGNAL(generalChanged())}
                                 << QVariant::fromValue(general) << QString{}
                                 << QVariantMap{{u"deleteOldModVersion"_s, u"false"_s},
                                                {u"includeModPrerelease"_s, u"true"_s}};
        QTest::newRow("paths") << QByteArray{"paths"} << QByteArray{SIGNAL(pathsChanged())}
                               << QVariant::fromValue(paths) << u"Paths"_s
                               << QVariantMap{{u"gameConfig"_s, u"/opt/vs-config"_s},
                                              {u"gameExe"_s, u"/opt/vs/game"_s}};
        QTest::newRow("appearance") << QByteArray{"appearance"} << QByteArray{SIGNAL(appearanceChanged())}
                                    << QVariant::fromValue(appearance) << u"Appearance"_s
                                    << QVariantMap{{u"theme"_s, u"light"_s}, {u"accentIndex"_s, u"3"_s}};
    }

    void sectionPropertiesNotifyOnlyOnChange() {
        QFETCH(QByteArray, property);
        QFETCH(QByteArray, signalName);
        QFETCH(QVariant, value);
        QFETCH(QString, group);
        QFETCH(QVariantMap, expectedSaved);

        vsmm::Config config;
        QSignalSpy spy{&config, signalName.constData()};
        QVERIFY(spy.isValid());

        QVERIFY(config.setProperty(property.constData(), value));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(config.property(property.constData()), value);

        config.save();
        QSettings saved;
        saved.beginGroup(group);
        for (const auto &[key, expected] : expectedSaved.asKeyValueRange()) {
            QCOMPARE(saved.value(key).toString(), expected.toString());
        }

        // same value must not re-notify or QML rebinds for nothing
        QVERIFY(config.setProperty(property.constData(), value));
        QCOMPARE(spy.count(), 1);
    }

    void gameConfigPathChangedTracksOnlyGameConfig() {
        vsmm::Config config;
        QSignalSpy pathsSpy{&config, &vsmm::Config::pathsChanged};
        QSignalSpy gameConfigSpy{&config, &vsmm::Config::gameConfigPathChanged};

        QVERIFY(
            config.setProperty("paths", QVariant::fromValue(vsmm::PathSettings{.gameConfig = u"/opt/vs-config"_s})));
        QCOMPARE(pathsSpy.count(), 1);
        QCOMPARE(gameConfigSpy.count(), 1);

        // another key changed, so paths changed but the game config dir did not
        QVERIFY(config.setProperty("paths", QVariant::fromValue(vsmm::PathSettings{.gameConfig = u"/opt/vs-config"_s,
                                                                                   .gameExe = u"/opt/mods"_s})));
        QCOMPARE(pathsSpy.count(), 2);
        QCOMPARE(gameConfigSpy.count(), 1);

        QVERIFY(config.setProperty("paths", QVariant::fromValue(vsmm::PathSettings{.gameConfig = u"/opt/other"_s})));
        QCOMPARE(pathsSpy.count(), 3);
        QCOMPARE(gameConfigSpy.count(), 2);
    }

    void validateRejectsBadGamePath_data() {
        QTest::addColumn<QVariant>("gameConfig");
        QTest::newRow("game-config-path-missing") << QVariant{};
        QTest::newRow("game-config-path-empty") << QVariant{u""_s};
        QTest::newRow("game-config-dir-absent") << QVariant{u"/nope/does/not/exist"_s};
    }

    void validateRejectsBadGamePath() {
        QFETCH(QVariant, gameConfig);
        if (gameConfig.isValid()) {
            QSettings{}.setValue(u"Paths/gameConfig"_s, gameConfig);
        }

        vsmm::Config config;
        QSignalSpy pathsSpy{&config, &vsmm::Config::pathsChanged};
        QSignalSpy gameConfigSpy{&config, &vsmm::Config::gameConfigPathChanged};

        QTest::ignoreMessage(QtCriticalMsg, "Config game path does not exist");
        config.validate();

        QCOMPARE(pathsSpy.count(), 0);
        QCOMPARE(gameConfigSpy.count(), 0);
    }

    void validateConfig() {
        QTemporaryDir gameDir;
        QVERIFY(gameDir.isValid());
        QSettings{}.setValue(u"Paths/gameConfig"_s, gameDir.path());

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
