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

#include "ModStoreTestUtils.hpp"

#include <RealComponents.hpp>

#include <QLoggingCategory>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;
using LoadType = vsmm::ModStore::ModLoadType;

class ModStoreIntegrationTest : public QObject {
    Q_OBJECT

    // store warnings stay visible, its info chatter is pinned by the unit suite
    static void onlyStoreWarnings() {
        QLoggingCategory::setFilterRules(u"*=false\nmodstore=true\nmodstore.info=false\nmodstore.debug=false"_s);
    }

    std::unique_ptr<RealComponents> mApp;

    [[nodiscard]] vsmm::ModStore &store() const { return mApp->store(); }

    // returns the zip path, only a few tests need it
    QString addMod(const QDir &from, const QString &id, const QString &version,
                   LoadType loadType = LoadType::Init) const {
        const QFileInfo zip = writeStubZip(from, u"%1-%2.zip"_s.arg(id, version));
        store().add(localInfo(id, version, zip), loadType);
        return zip.absoluteFilePath();
    }

    // what Config last wrote to disk, not what it holds in memory
    [[nodiscard]] static QStringList savedFavorites() {
        const QJsonDocument saved = QJsonDocument::fromJson(readFile(RealComponents::configFilePath()));
        return saved[vsmm::IConfig::FAVORITES_KEY_NAME].toVariant().toStringList();
    }

    [[nodiscard]] bool hasUpdate(const QString &id) const {
        const vsmm::ModEntry *mod = store().find(id);
        return mod && mod->hasUpdate();
    }

  private slots:
    void initTestCase() {
        // the version comes from a fake game exe, a POSIX shell script
        SKIP_WITHOUT_POSIX_SHELL();
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(RealComponents::configDir().contains("qttest"_L1), qPrintable(RealComponents::configDir()));
    }

    void cleanupTestCase() { QStandardPaths::setTestModeEnabled(false); }

    void init() {
        onlyStoreWarnings();
        mApp = std::make_unique<RealComponents>();
    }

    void cleanup() { mApp.reset(); }

    // the star is saved through the real config file and read back on the next launch
    void favoritesPersistAcrossARestart() {
        QVERIFY(mApp->start());
        addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);
        store().setFavorite(u"carryon"_s, true);

        QVERIFY(mApp->restart());

        QCOMPARE(mApp->config().getFavorites(), QStringList{u"carryon"_s});
        addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);
        QVERIFY(store().find(u"carryon"_s)->isFavorite());
    }

    void unfavoritedModIsDroppedFromTheConfigFile() {
        RealComponents::Options options;
        options.mFavorites = {u"carryon"_s, u"betterruins"_s};
        QVERIFY(mApp->start(options));
        addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);
        addMod(mApp->modsDir(), u"betterruins"_s, u"1.0.0"_s);
        QVERIFY(store().find(u"carryon"_s)->isFavorite());

        store().setFavorite(u"carryon"_s, false);

        QCOMPARE(savedFavorites(), QStringList{u"betterruins"_s});
    }

    void oldZipHandlingFollowsTheConfigFile_data() {
        QTest::addColumn<bool>("deleteOldModVersion");

        QTest::newRow("keep") << false;
        QTest::newRow("trash") << true;
    }

    void oldZipHandlingFollowsTheConfigFile() {
        QFETCH(const bool, deleteOldModVersion);
        RealComponents::Options options;
        options.mGeneral[vsmm::GeneralSettings::DELETE_OLD_MOD_VERSION_KEY] = deleteOldModVersion;
        QVERIFY(mApp->start(options));
        const QString oldZip = addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);
        const auto restoreTestMode = qScopeGuard([] { QStandardPaths::setTestModeEnabled(true); });
        if (deleteOldModVersion) {
            // test mode moves the home trash under ~/.qttest where moveToTrash fails, so trash for real
            QStandardPaths::setTestModeEnabled(false);
            if (!trashWorksIn(mApp->stagingDir())) {
                QSKIP("No trash available for the temp dir filesystem");
            }
        } else {
            // the default is true, so reading the file wrong would show up as a trash attempt
            QTest::failOnWarning(QRegularExpression{u"to trash"_s});
        }

        addMod(mApp->stagingDir(), u"carryon"_s, u"1.1.0"_s, LoadType::Update);

        QCOMPARE(str(store().find(u"carryon"_s)->getVersion()), u"1.1.0"_s);
        QCOMPARE(QFile::exists(oldZip), !deleteOldModVersion);
    }

    void prereleaseUpdatesFollowTheConfigFile_data() {
        QTest::addColumn<bool>("includeModPrerelease");

        QTest::newRow("included") << true;
        QTest::newRow("excluded") << false;
    }

    void prereleaseUpdatesFollowTheConfigFile() {
        QFETCH(const bool, includeModPrerelease);
        RealComponents::Options options;
        options.mGeneral[vsmm::GeneralSettings::INCLUDE_MOD_PRERELEASE_KEY] = includeModPrerelease;
        QVERIFY(mApp->start(options));
        addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);

        store().updateOnline(u"carryon"_s, modJson({release(u"1.1.0-rc.1"_s, {u"1.22.0"_s})}));

        QCOMPARE(hasUpdate(u"carryon"_s), includeModPrerelease);
        QCOMPARE(store().modUpdatesCount(), includeModPrerelease ? 1 : 0);
    }

    void updateDetectionMatchesTheGameMinorVersion_data() {
        QTest::addColumn<QString>("supportedGameVersion");
        QTest::addColumn<bool>("expectUpdate");

        QTest::newRow("same-minor") << u"1.22.0"_s << true;
        QTest::newRow("older-minor") << u"1.21.0"_s << false;
        QTest::newRow("newer-minor") << u"1.23.0"_s << false;
    }

    // the exe reports 1.22.5, only patch numbers are treated as compatible
    void updateDetectionMatchesTheGameMinorVersion() {
        QFETCH(const QString, supportedGameVersion);
        QFETCH(const bool, expectUpdate);
        QVERIFY(mApp->start());
        QCOMPARE(str(*mApp->gameMngr().getGameVersion()), u"1.22.5"_s);
        addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);

        store().updateOnline(u"carryon"_s, modJson({release(u"1.1.0"_s, {supportedGameVersion})}));

        QCOMPARE(hasUpdate(u"carryon"_s), expectUpdate);
    }

    // the scan still runs, but no release can match an unknown game
    void unreadableGameVersionDisablesUpdateDetection() {
        RealComponents::Options options;
        options.mGameVersionOutput = u"not a version"_s;
        QVERIFY(mApp->start(options));
        QVERIFY(!mApp->gameMngr().getGameVersion());
        addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);

        store().updateOnline(u"carryon"_s, modJson({release(u"1.1.0"_s, {u"1.22.0"_s})}));

        QVERIFY(!hasUpdate(u"carryon"_s));
    }

    void guiInstallLandsInTheFirstModsDirFromClientSettings() {
        RealComponents::Options options;
        options.mExtraModsDirs = {u"mods2"_s};
        QVERIFY(mApp->start(options));
        QCOMPARE(mApp->gameMngr().getModsDirs().size(), qsizetype{2});

        addMod(mApp->stagingDir(), u"carryon"_s, u"1.0.0"_s, LoadType::GUI);

        QVERIFY(mApp->modsDir().exists(u"carryon-1.0.0.zip"_s));
        QVERIFY(!mApp->dir(u"mods2"_s).exists(u"carryon-1.0.0.zip"_s));
        QCOMPARE(store().find(u"carryon"_s)->getFileInfo().absolutePath(), mApp->modsDir().absolutePath());
    }

    // Settings saving another game config dir goes Config -> GameMngr -> ModStore
    void switchingTheGameConfigReloadsTheStore() {
        QVERIFY(mApp->start());
        addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);
        const QDir otherGameDir = mApp->dir(u"other-game"_s);
        QVERIFY(mApp->root().mkpath(u"other-game"_s));
        QVERIFY(writeClientSettings(otherGameDir, clientSettings({mApp->stagingDir().absolutePath()})));
        const QSignalSpy reloading{&store(), &vsmm::IModStore::modsReloading};
        vsmm::PathSettings paths = mApp->config().paths();
        paths.gameConfig = otherGameDir.absolutePath();

        mApp->config().setPaths(paths);

        QTRY_COMPARE(reloading.count(), 1);
        QCOMPARE(store().modsCount(), 0);
        QVERIFY(store().isWorkPending());
        QCOMPARE(toPaths(mApp->gameMngr().getModsDirs()), QStringList{mApp->stagingDir().absolutePath()});
    }

    // another game install means another version, so the mods are rescanned and matched against it
    void newGameVersionRescans() {
        QVERIFY(mApp->start());
        addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);
        QVERIFY(mApp->root().mkpath(u"other-exe"_s));
        const QString exe = writeFakeGameExe(mApp->dir(u"other-exe"_s), u"1.23.0"_s);
        QVERIFY(!exe.isEmpty());
        const QSignalSpy reloading{&store(), &vsmm::IModStore::modsReloading};
        vsmm::PathSettings paths = mApp->config().paths();
        paths.gameExe = exe;

        mApp->config().setPaths(paths);

        QTRY_COMPARE(reloading.count(), 1);
        QCOMPARE(str(*mApp->gameMngr().getGameVersion()), u"1.23.0"_s);
        QCOMPARE(store().modsCount(), 0);

        addMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);
        store().updateOnline(u"carryon"_s, modJson({release(u"1.1.0"_s, {u"1.23.0"_s})}));
        QVERIFY(hasUpdate(u"carryon"_s));
    }
};

QTEST_GUILESS_MAIN(ModStoreIntegrationTest)
#include "ModStoreIntegrationTest.moc"
