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
#include <GameMngr.hpp>
#include <GameMngrTestUtils.hpp>
#include <ModEntryTestUtils.hpp>
#include <ModListModel.hpp>
#include <ModStore.hpp>

#include <QAbstractItemModelTester>
#include <QLoggingCategory>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;
using Roles = vsmm::ModListModel::Roles;
using LoadType = vsmm::ModStore::ModLoadType;

namespace {
constexpr auto GAME_VERSION = "1.22.5";

// first and last row of a rowsInserted/rowsRemoved emission, parent must be the root
[[nodiscard]] QPair<int, int> rowRange(const QList<QVariant> &args) {
    QTest::qVerify(!args.at(0).value<QModelIndex>().isValid(), "parent is root", "", __FILE__, __LINE__);
    return {args.at(1).toInt(), args.at(2).toInt()};
}

// top-left and bottom-right row of a dataChanged emission
[[nodiscard]] QPair<int, int> changedRows(const QList<QVariant> &args) {
    return {args.at(0).value<QModelIndex>().row(), args.at(1).value<QModelIndex>().row()};
}
} // namespace

class ModListModelIntegrationTest : public QObject {
    Q_OBJECT

    // only the model logs, the other components are pinned by their own suites
    static void onlyModelLogs() {
        QLoggingCategory::setFilterRules(u"*=false\nmodlistmodel=true\nmodlistmodel.debug=false"_s);
    }

    [[nodiscard]] static QString configDir() {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    [[nodiscard]] static QString configFilePath() {
        return configDir() + QDir::separator() + vsmm::IConfig::CONFIG_FILE_NAME;
    }

    // config.json as the app would find it, old zips are kept so nothing goes to the real trash
    [[nodiscard]] static bool writeConfig(const QString &gameConfigDir, const QString &gameExe) {
        if (!QDir().mkpath(configDir())) {
            return false;
        }
        const QJsonObject general{{vsmm::GeneralSettings::DELETE_OLD_MOD_VERSION_KEY, false}};
        const QJsonObject paths{{vsmm::PathSettings::GAME_CONFIG_KEY, gameConfigDir},
                                {vsmm::PathSettings::GAME_EXE_KEY, gameExe}};
        return writeFile(configFilePath(), QJsonDocument{QJsonObject{{vsmm::GeneralSettings::KEY_NAME, general},
                                                                     {vsmm::PathSettings::KEY_NAME, paths}}}
                                               .toJson());
    }

    std::unique_ptr<QTemporaryDir> mTempDir;
    std::unique_ptr<vsmm::Config> mConfig;
    std::unique_ptr<vsmm::GameMngr> mGameMngr;
    std::unique_ptr<vsmm::ModStore> mStore;
    std::unique_ptr<vsmm::ModListModel> mModel;
    std::unique_ptr<QAbstractItemModelTester> mTester;

    [[nodiscard]] QDir gameDir() const { return QDir{mTempDir->filePath(u"game"_s)}; }
    [[nodiscard]] QDir modsDir() const { return QDir{mTempDir->filePath(u"mods"_s)}; }
    // stands in for wherever a zip is picked up from, a download or a second mods folder
    [[nodiscard]] QDir stagingDir() const { return QDir{mTempDir->filePath(u"staging"_s)}; }

    void addMod(const QDir &from, const QString &id, const QString &version, LoadType loadType = LoadType::Init) {
        mStore->add(localInfo(id, version, writeStubZip(from, u"%1-%2.zip"_s.arg(id, version))), loadType);
    }

    void addThree() {
        addMod(modsDir(), u"carryon"_s, u"1.0.0"_s);
        addMod(modsDir(), u"betterruins"_s, u"1.0.0"_s);
        addMod(modsDir(), u"animalcages"_s, u"1.0.0"_s);
    }

    [[nodiscard]] QVariant role(int row, int role) const { return mModel->data(mModel->index(row), role); }

    // what the view would show, top to bottom
    [[nodiscard]] QStringList ids() const {
        QStringList ids;
        for (int row = 0; row < mModel->rowCount(QModelIndex{}); ++row) {
            ids.append(role(row, Roles::IdRole).toString());
        }
        return ids;
    }

  private slots:
    void initTestCase() {
        // the version comes from a fake game exe, a POSIX shell script
        SKIP_WITHOUT_POSIX_SHELL();
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(configDir().contains("qttest"_L1), qPrintable(configDir()));
    }

    void cleanupTestCase() { QStandardPaths::setTestModeEnabled(false); }

    void init() {
        onlyModelLogs();

        mTempDir = std::make_unique<QTemporaryDir>();
        QVERIFY(mTempDir->isValid());
        for (const auto &dir : {u"game"_s, u"mods"_s, u"staging"_s}) {
            QVERIFY(QDir{mTempDir->path()}.mkpath(dir));
        }
        QVERIFY(writeClientSettings(gameDir(), clientSettings({modsDir().absolutePath()})));
        const QString exe = writeFakeGameExe(QDir{mTempDir->path()}, QString::fromLatin1(GAME_VERSION));
        QVERIFY(!exe.isEmpty());
        QVERIFY(writeConfig(gameDir().absolutePath(), exe));

        // wired the way App does it
        mConfig = std::make_unique<vsmm::Config>();
        mGameMngr = std::make_unique<vsmm::GameMngr>();
        mGameMngr->setConfig(mConfig.get());
        mStore = std::make_unique<vsmm::ModStore>();
        mStore->setConfig(mConfig.get());
        mStore->setGameMngr(mGameMngr.get());
        mModel = std::make_unique<vsmm::ModListModel>();
        mModel->setStore(mStore.get());
        // checks every insert, removal and data() call below against the QAbstractItemModel contract
        mTester = std::make_unique<QAbstractItemModelTester>(mModel.get(),
                                                             QAbstractItemModelTester::FailureReportingMode::QtTest);

        // the first scan is announced only once the version read settled
        const QSignalSpy scan{mGameMngr.get(), &vsmm::IGameMngr::modsDirsChanged};
        mConfig->validate();
        QTRY_COMPARE(scan.count(), 1);
        QVERIFY(mGameMngr->getGameVersion());
        QCOMPARE(toPaths(mGameMngr->getModsDirs()), QStringList{modsDir().absolutePath()});
    }

    void cleanup() {
        mTester.reset();
        mModel.reset();
        mStore.reset();
        mGameMngr.reset();
        // saves on destruction, so remove the file after it
        mConfig.reset();
        QFile::remove(configFilePath());
        mTempDir.reset();
    }

    // sorting belongs to the proxy, the model keeps scan order
    void initScanShowsModsInScanOrder() {
        addThree();

        QCOMPARE(ids(), QStringList({u"carryon"_s, u"betterruins"_s, u"animalcages"_s}));
        QCOMPARE(mModel->rowCount(QModelIndex{}), mStore->modsCount());
        QCOMPARE(role(1, Roles::NameRole).toString(), u"betterruins (local)"_s);
        QCOMPARE(role(1, Roles::VersionRole).toString(), u"1.0.0"_s);
    }

    // the zip already in the model wins, the store keeps it and tells nobody
    void olderDuplicateOnInitLeavesTheRowAlone() {
        addMod(modsDir(), u"carryon"_s, u"1.1.0"_s);
        const QSignalSpy inserted{mModel.get(), &QAbstractItemModel::rowsInserted};
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        addMod(stagingDir(), u"carryon"_s, u"1.0.0"_s);

        QCOMPARE(inserted.count(), 0);
        QCOMPARE(changed.count(), 0);
        QCOMPARE(ids(), QStringList{u"carryon"_s});
        QCOMPARE(role(0, Roles::VersionRole).toString(), u"1.1.0"_s);
    }

    // ModStore announces the replacement as an update, so the row stays where it was
    void newerDuplicateOnInitRefreshesTheRowInPlace() {
        addThree();
        const QSignalSpy inserted{mModel.get(), &QAbstractItemModel::rowsInserted};
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        addMod(stagingDir(), u"betterruins"_s, u"1.1.0"_s);

        QCOMPARE(inserted.count(), 0);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changedRows(changed.at(0)), qMakePair(1, 1));
        QCOMPARE(ids(), QStringList({u"carryon"_s, u"betterruins"_s, u"animalcages"_s}));
        QCOMPARE(role(1, Roles::VersionRole).toString(), u"1.1.0"_s);
    }

    void modWithoutAnIdAddsNoRow() {
        const QSignalSpy inserted{mModel.get(), &QAbstractItemModel::rowsInserted};

        mStore->add(localInfo(QString{}, u"1.0.0"_s, writeStubZip(modsDir(), u"noid.zip"_s)), LoadType::Init);

        QCOMPARE(inserted.count(), 0);
        QCOMPARE(mModel->rowCount(QModelIndex{}), 0);
    }

    // the store installs into the first mods dir GameMngr read from clientsettings
    void guiInstallAddsARow() {
        const QSignalSpy inserted{mModel.get(), &QAbstractItemModel::rowsInserted};

        addMod(stagingDir(), u"carryon"_s, u"1.0.0"_s, LoadType::GUI);

        QCOMPARE(inserted.count(), 1);
        QCOMPARE(rowRange(inserted.at(0)), qMakePair(0, 0));
        QCOMPARE(ids(), QStringList{u"carryon"_s});
        QVERIFY(modsDir().exists(u"carryon-1.0.0.zip"_s));
    }

    void knownModInstallRefreshesItsRow_data() {
        QTest::addColumn<int>("loadType");

        QTest::newRow("gui") << static_cast<int>(LoadType::GUI);
        QTest::newRow("update-download") << static_cast<int>(LoadType::Update);
    }

    // both paths go through ModStore::updateMod, the row refreshes instead of a second one appearing
    void knownModInstallRefreshesItsRow() {
        QFETCH(const int, loadType);
        addThree();
        const QSignalSpy inserted{mModel.get(), &QAbstractItemModel::rowsInserted};
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        addMod(stagingDir(), u"animalcages"_s, u"1.1.0"_s, static_cast<LoadType>(loadType));

        QCOMPARE(inserted.count(), 0);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changedRows(changed.at(0)), qMakePair(2, 2));
        QCOMPARE(mModel->rowCount(QModelIndex{}), 3);
        QCOMPARE(role(2, Roles::VersionRole).toString(), u"1.1.0"_s);
    }

    void staleUpdateLeavesTheRowAlone_data() {
        QTest::addColumn<QString>("version");

        QTest::newRow("same") << u"1.0.0"_s;
        QTest::newRow("older") << u"0.9.0"_s;
    }

    void staleUpdateLeavesTheRowAlone() {
        QFETCH(const QString, version);
        addMod(modsDir(), u"carryon"_s, u"1.0.0"_s);
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        addMod(stagingDir(), u"carryon"_s, version, LoadType::Update);

        QCOMPARE(changed.count(), 0);
        QCOMPARE(role(0, Roles::VersionRole).toString(), u"1.0.0"_s);
    }

    // update detection runs against the version GameMngr read from the game exe
    void onlineInfoRefreshesTheRow() {
        addThree();
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        mStore->updateOnline(u"carryon"_s, modJson({release(u"1.1.0"_s, {u"1.22.0"_s})}));

        QCOMPARE(changed.count(), 1);
        QCOMPARE(changedRows(changed.at(0)), qMakePair(0, 0));
        QCOMPARE(role(0, Roles::NameRole).toString(), u"Carry On"_s);
        QCOMPARE(role(0, Roles::LatestVersionRole).toString(), u"1.1.0"_s);
        QCOMPARE(role(0, Roles::HasUpdateRole).toBool(), true);
        QCOMPARE(role(1, Roles::HasUpdateRole).toBool(), false);
    }

    void favoriteFromConfigShowsOnFirstAdd_data() {
        QTest::addColumn<int>("loadType");

        QTest::newRow("init") << static_cast<int>(LoadType::Init);
        QTest::newRow("gui") << static_cast<int>(LoadType::GUI);
    }

    // favorites are read when the config is set, so this one needs a store of its own
    void favoriteFromConfigShowsOnFirstAdd() {
        QFETCH(const int, loadType);
        mConfig->setFavorites({u"betterruins"_s});
        vsmm::ModStore store;
        store.setConfig(mConfig.get());
        store.setGameMngr(mGameMngr.get());
        vsmm::ModListModel model;
        model.setStore(&store);
        const QAbstractItemModelTester tester{&model, QAbstractItemModelTester::FailureReportingMode::QtTest};

        for (const auto &id : {u"carryon"_s, u"betterruins"_s}) {
            store.add(localInfo(id, u"1.0.0"_s, writeStubZip(stagingDir(), u"%1.zip"_s.arg(id))),
                      static_cast<LoadType>(loadType));
        }

        QCOMPARE(model.rowCount(QModelIndex{}), 2);
        QCOMPARE(model.data(model.index(0), Roles::FavoriteRole).toBool(), false);
        QCOMPARE(model.data(model.index(1), Roles::FavoriteRole).toBool(), true);
    }

    void setFavoriteRefreshesTheRow() {
        addThree();
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        mStore->setFavorite(u"betterruins"_s, true);

        QCOMPARE(changed.count(), 1);
        QCOMPARE(changedRows(changed.at(0)), qMakePair(1, 1));
        QCOMPARE(role(1, Roles::FavoriteRole).toBool(), true);
    }

    void favoriteSurvivesAReplacement_data() {
        QTest::addColumn<int>("loadType");

        QTest::newRow("init-newer-duplicate") << static_cast<int>(LoadType::Init);
        QTest::newRow("gui-install") << static_cast<int>(LoadType::GUI);
        QTest::newRow("update-download") << static_cast<int>(LoadType::Update);
    }

    // every path that swaps the entry for a newer zip has to carry the star over
    void favoriteSurvivesAReplacement() {
        QFETCH(const int, loadType);
        addThree();
        mStore->setFavorite(u"animalcages"_s, true);

        addMod(stagingDir(), u"animalcages"_s, u"1.1.0"_s, static_cast<LoadType>(loadType));

        QCOMPARE(role(2, Roles::VersionRole).toString(), u"1.1.0"_s);
        QCOMPARE(role(2, Roles::FavoriteRole).toBool(), true);
    }

    // the store announces before erasing, so the view can still read the row it is dropping
    void removeDropsTheRowWhileTheEntryIsStillReadable() {
        addThree();
        QString readWhileRemoving;
        connect(mModel.get(), &QAbstractItemModel::rowsAboutToBeRemoved, this,
                [this, &readWhileRemoving](const QModelIndex &, int first, int) {
                    readWhileRemoving = role(first, Roles::IdRole).toString();
                });

        mStore->remove(u"betterruins"_s);

        QCOMPARE(readWhileRemoving, u"betterruins"_s);
        QCOMPARE(ids(), QStringList({u"carryon"_s, u"animalcages"_s}));
        QCOMPARE(role(1, Roles::NameRole).toString(), u"animalcages (local)"_s);
    }

    // the row stays rather than pointing at a file the store failed to delete
    void failedRemoveKeepsTheRow() {
        addThree();
        QVERIFY(QFile::remove(mStore->find(u"betterruins"_s)->getFileInfo().absoluteFilePath()));
        const QSignalSpy removing{mModel.get(), &QAbstractItemModel::rowsAboutToBeRemoved};

        mStore->remove(u"betterruins"_s);

        QCOMPARE(removing.count(), 0);
        QCOMPARE(ids(), QStringList({u"carryon"_s, u"betterruins"_s, u"animalcages"_s}));
    }

    // same announce-before-erase contract as remove, for every row at once
    void reloadEmptiesTheModelAndARescanRefillsIt() {
        addThree();
        const QSignalSpy removedRows{mModel.get(), &QAbstractItemModel::rowsRemoved};
        QStringList readWhileRemoving;
        connect(mModel.get(), &QAbstractItemModel::rowsAboutToBeRemoved, this,
                [this, &readWhileRemoving](const QModelIndex &, int first, int last) {
                    for (int row = first; row <= last; ++row) {
                        readWhileRemoving.append(role(row, Roles::IdRole).toString());
                    }
                });

        mStore->reload();

        QCOMPARE(readWhileRemoving, QStringList({u"carryon"_s, u"betterruins"_s, u"animalcages"_s}));
        QCOMPARE(mModel->rowCount(QModelIndex{}), 0);
        QCOMPARE(removedRows.count(), 1);
        QCOMPARE(rowRange(removedRows.at(0)), qMakePair(0, 2));

        addThree();
        QCOMPARE(ids(), QStringList({u"carryon"_s, u"betterruins"_s, u"animalcages"_s}));
    }

    // Settings saving another game config dir goes Config -> GameMngr -> ModStore, the old rows must not linger
    void switchingTheGameConfigEmptiesTheModel() {
        addThree();
        const QDir otherGameDir{mTempDir->filePath(u"other-game"_s)};
        QVERIFY(QDir{mTempDir->path()}.mkpath(u"other-game"_s));
        QVERIFY(writeClientSettings(otherGameDir, clientSettings({stagingDir().absolutePath()})));
        vsmm::PathSettings paths = mConfig->paths();
        paths.gameConfig = otherGameDir.absolutePath();

        mConfig->setPaths(paths);

        QTRY_COMPARE(mModel->rowCount(QModelIndex{}), 0);
        QCOMPARE(toPaths(mGameMngr->getModsDirs()), QStringList{stagingDir().absolutePath()});
    }
};

QTEST_GUILESS_MAIN(ModListModelIntegrationTest)
#include "ModListModelIntegrationTest.moc"
