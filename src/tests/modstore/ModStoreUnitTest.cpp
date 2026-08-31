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

#include <ConfigMock.hpp>
#include <GameMngrMock.hpp>
#include <ModStore.hpp>

#include <QLoggingCategory>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>
#include <optional>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

class ModStoreUnitTest : public QObject {
    Q_OBJECT

    static constexpr auto GAME_VERSION = "1.22.5";

    // a QString, updateOnline and find take one, and QStringView cannot be built from a latin1 literal
    [[nodiscard]] static QString modId() { return u"carryon"_s; }

    // only this component logs, and no tracing unless a test asks for it
    static void onlyStoreLogs() { QLoggingCategory::setFilterRules(u"*=false\nmodstore=true\nmodstore.debug=false"_s); }

    std::unique_ptr<QTemporaryDir> mTempDir;
    std::unique_ptr<vsmm::ConfigMock> mConfig;
    std::unique_ptr<vsmm::GameMngrMock> mGameMngr;
    std::unique_ptr<vsmm::ModStore> mStore;

    [[nodiscard]] QDir modsDir() const { return QDir{mTempDir->filePath(u"mods"_s)}; }
    // stands in for wherever a zip is picked up from, a download or a second mods folder
    [[nodiscard]] QDir stagingDir() const { return QDir{mTempDir->filePath(u"staging"_s)}; }

    void setComponents() {
        mStore->setConfig(mConfig.get());
        mStore->setGameMngr(mGameMngr.get());
    }

    void addMod(const QDir &from, const QString &id, const QString &version,
                vsmm::ModStore::ModLoadType loadType = vsmm::ModStore::ModLoadType::Init) {
        mStore->add(localInfo(id, version, writeStubZip(from, u"%1-%2.zip"_s.arg(id, version))), loadType);
    }

    // the online answer the API would give for a mod that has a newer release for this game version
    [[nodiscard]] static QJsonObject withNewerRelease() { return modJson({release(u"1.1.0"_s, {u"1.22.0"_s})}); }
    // a release older than what is installed, so the mod reads as current without an empty-releases warning
    [[nodiscard]] static QJsonObject withoutNewerRelease() { return modJson({release(u"0.9.0"_s, {u"1.22.0"_s})}); }

  private slots:
    void init() {
        onlyStoreLogs();

        mTempDir = std::make_unique<QTemporaryDir>();
        QVERIFY(mTempDir->isValid());
        QVERIFY(QDir{mTempDir->path()}.mkpath(u"mods"_s));
        QVERIFY(QDir{mTempDir->path()}.mkpath(u"staging"_s));

        mConfig = std::make_unique<vsmm::ConfigMock>();
        mGameMngr = std::make_unique<vsmm::GameMngrMock>();
        mStore = std::make_unique<vsmm::ModStore>();

        // primed before wiring, modsDirsChanged would otherwise reload the store mid-setup
        mGameMngr->setModsDirs({modsDir()});
        mGameMngr->setGameVersion(ver(GAME_VERSION));
    }

    void cleanup() {
        mStore.reset();
        mGameMngr.reset();
        mConfig.reset();
        mTempDir.reset();
    }

    void freshStoreIsEmptyAndWaitsForTheFirstScan() {
        QCOMPARE(mStore->modsCount(), 0);
        QCOMPARE(mStore->modUpdatesCount(), 0);
        QVERIFY(!mStore->contains(modId()));
        QVERIFY(mStore->find(modId()) == nullptr);
        // nothing has been scanned yet, so the GUI must not offer a reload
        QVERIFY(mStore->isWorkPending());
    }

    // set-once: a second call must not rewire, or reloads would arrive from a stale manager
    void secondGameMngrIsRefused() {
        setComponents();
        vsmm::GameMngrMock other;

        QTest::ignoreMessage(QtWarningMsg, "GameMngr already set");
        mStore->setGameMngr(&other);

        QSignalSpy reloading{mStore.get(), &vsmm::ModStore::modsReloading};
        other.setModsDirs({stagingDir()});
        QCOMPARE(reloading.count(), 0);

        QTest::ignoreMessage(QtInfoMsg, "Reloading mods");
        mGameMngr->setModsDirs({modsDir()});
        QCOMPARE(reloading.count(), 1);
    }

    // a mod picked in the GUI comes from outside the mods dir, the store installs it
    void guiAddCopiesTheZipIntoTheModsDirAndPointsTheEntryAtIt() {
        setComponents();
        QStringList added;
        connect(mStore.get(), &vsmm::ModStore::modAdded, this,
                [&added](QStringView id) { added.append(id.toString()); });
        QSignalSpy changed{mStore.get(), &vsmm::ModStore::modsChanged};

        const QFileInfo zip = writeStubZip(stagingDir(), u"carryon-1.0.0.zip"_s);
        QVERIFY(zip.exists());
        mStore->add(localInfo(modId(), u"1.0.0"_s, zip), vsmm::ModStore::ModLoadType::GUI);

        QCOMPARE(added, QStringList{modId()});
        QCOMPARE(changed.count(), 1);
        QCOMPARE(mStore->modsCount(), 1);
        QCOMPARE(mStore->property("installedModsCount").toInt(), 1);
        const vsmm::ModEntry *mod = mStore->find(modId());
        QVERIFY(mod != nullptr);
        QCOMPARE(str(mod->getVersion()), u"1.0.0"_s);
        QVERIFY(modsDir().exists(u"carryon-1.0.0.zip"_s));
        // the entry names the copy the store owns, remove() deletes whatever it points at
        QCOMPARE(mod->getFileInfo().absoluteFilePath(), modsDir().absoluteFilePath(u"carryon-1.0.0.zip"_s));
    }

    void modWithoutAnIdIsRefused() {
        setComponents();
        const QFileInfo zip = writeStubZip(stagingDir(), u"nameless.zip"_s);
        QTest::ignoreMessage(QtWarningMsg,
                             qPrintable(u"Refusing mod without an id from %1"_s.arg(zip.absoluteFilePath())));

        mStore->add(localInfo(QString{}, u"1.0.0"_s, zip), vsmm::ModStore::ModLoadType::GUI);

        QCOMPARE(mStore->modsCount(), 0);
        QVERIFY(!modsDir().exists(u"nameless.zip"_s));
    }

    // no mods dir means nowhere to put the zip, so the entry is dropped rather than stored unbacked
    void guiAddIsRefusedWhenNoModsDirIsConfigured() {
        mGameMngr->setModsDirs({});
        setComponents();
        QTest::ignoreMessage(QtCriticalMsg, "No mods dir to add carryon@1.0.0 to");

        addMod(stagingDir(), modId(), u"1.0.0"_s, vsmm::ModStore::ModLoadType::GUI);

        QCOMPARE(mStore->modsCount(), 0);
    }

    // another mod already owns the file name, the zip still has to land inside the mods dir
    void guiAddUsesAUniqueNameWhenTheFileNameIsTaken() {
        setComponents();
        QVERIFY(writeStubZip(modsDir(), u"mod.zip"_s).exists());
        const QFileInfo zip = writeStubZip(stagingDir(), u"mod.zip"_s);

        mStore->add(localInfo(modId(), u"1.0.0"_s, zip), vsmm::ModStore::ModLoadType::GUI);

        QVERIFY(mStore->contains(modId()));
        const QFileInfo installed = mStore->find(modId())->getFileInfo();
        QCOMPARE(installed.absolutePath(), modsDir().absolutePath());
        QVERIFY(installed.exists());
        // the other mod's zip is untouched, the new one sits beside it under a generated name
        QVERIFY(installed.fileName() != u"mod.zip"_s);
        QCOMPARE(modsDir().entryList(QStringList{u"*.zip"_s}, QDir::Files).size(), 2);
    }

    // a scanned zip is already installed, the init path must not touch the disk at all
    void initStoresTheScannedZipWhereItWasFound() {
        setComponents();
        const QFileInfo zip = writeStubZip(modsDir(), u"carryon-1.0.0.zip"_s);

        mStore->add(localInfo(modId(), u"1.0.0"_s, zip), vsmm::ModStore::ModLoadType::Init);

        QCOMPARE(mStore->find(modId())->getFileInfo().absoluteFilePath(), zip.absoluteFilePath());
        // no second copy under a generated name either
        QCOMPARE(modsDir().entryList(QStringList{u"*.zip"_s}, QDir::Files), QStringList{u"carryon-1.0.0.zip"_s});
    }

    // an entry pointing at a file that was never installed would delete the user's own copy on remove()
    void guiAddIsDroppedWhenTheZipCannotBeInstalled() {
        SKIP_WITHOUT_DIR_PERMISSIONS();
        setComponents();
        const QFileInfo zip = writeStubZip(stagingDir(), u"carryon-1.0.0.zip"_s);
        QVERIFY(QFile::setPermissions(modsDir().absolutePath(), QFileDevice::ReadOwner | QFileDevice::ExeOwner));
        QTest::ignoreMessage(QtCriticalMsg, qPrintable(u"Failed to copy mod carryon@1.0.0 to %1"_s.arg(
                                                modsDir().absoluteFilePath(u"carryon-1.0.0.zip"_s))));

        mStore->add(localInfo(modId(), u"1.0.0"_s, zip), vsmm::ModStore::ModLoadType::GUI);

        // restored before the assertions, cleanup has to be able to delete the dir
        QVERIFY(QFile::setPermissions(modsDir().absolutePath(),
                                      QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        QVERIFY(!mStore->contains(modId()));
        QCOMPARE(mStore->modsCount(), 0);
    }

    // two versions of one mod can sit in the mods folder, the newer one already in the store wins
    void olderDuplicateIsIgnored() {
        setComponents();
        addMod(modsDir(), modId(), u"1.2.0"_s);
        QStringList updated;
        connect(mStore.get(), &vsmm::ModStore::modUpdated, this,
                [&updated](QStringView id) { updated.append(id.toString()); });

        QTest::ignoreMessage(QtInfoMsg,
                             qPrintable(u"Mod carryon@1.0.0 already has newer version in mods folder %1"_s.arg(
                                 modsDir().absolutePath())));
        addMod(modsDir(), modId(), u"1.0.0"_s);

        QCOMPARE(mStore->modsCount(), 1);
        QCOMPARE(str(mStore->find(modId())->getVersion()), u"1.2.0"_s);
        QVERIFY(updated.isEmpty());
    }

    // the scan finds both versions in the folder, the entry follows the newer zip and no file moves
    void initReplacesTheEntryWithTheNewerZipWithoutTouchingDisk() {
        setComponents();
        const QFileInfo older = writeStubZip(modsDir(), u"carryon-1.0.0.zip"_s);
        mStore->add(localInfo(modId(), u"1.0.0"_s, older), vsmm::ModStore::ModLoadType::Init);

        QStringList updated;
        connect(mStore.get(), &vsmm::ModStore::modUpdated, this,
                [&updated](QStringView id) { updated.append(id.toString()); });

        QTest::ignoreMessage(QtInfoMsg, "Got newer version of mod carryon@1.1.0, replacing");
        addMod(modsDir(), modId(), u"1.1.0"_s);

        QCOMPARE(updated, QStringList{modId()});
        QCOMPARE(mStore->modsCount(), 1);
        const vsmm::ModEntry *mod = mStore->find(modId());
        QCOMPARE(str(mod->getVersion()), u"1.1.0"_s);
        QCOMPARE(mod->getFileInfo().absoluteFilePath(), modsDir().absoluteFilePath(u"carryon-1.1.0.zip"_s));
        // a scan never deletes, the superseded zip is still there for the next one to find
        QVERIFY(QFile::exists(older.absoluteFilePath()));
    }

    // favorites are read once when the config is set, long before the scan finds the mod
    void favoriteFromTheConfigIsAppliedWhenTheModShowsUp() {
        mConfig->setFavorites({modId()});
        setComponents();

        addMod(modsDir(), modId(), u"1.0.0"_s);

        QVERIFY(mStore->find(modId())->isFavorite());
    }

    void setFavoriteWritesTheWholeSetBackToTheConfig() {
        setComponents();
        addMod(modsDir(), modId(), u"1.0.0"_s);
        QStringList updated;
        connect(mStore.get(), &vsmm::ModStore::modUpdated, this,
                [&updated](QStringView id) { updated.append(id.toString()); });

        mStore->setFavorite(modId(), true);

        QVERIFY(mStore->find(modId())->isFavorite());
        QCOMPARE(mConfig->getFavorites(), QStringList{modId()});
        QCOMPARE(updated, QStringList{modId()});

        mStore->setFavorite(modId(), false);

        QVERIFY(!mStore->find(modId())->isFavorite());
        QVERIFY(mConfig->getFavorites().isEmpty());
    }

    void onlineInfoForAnUnknownModIsRefused() {
        setComponents();
        const QString unknown = u"ghost"_s;
        QTest::ignoreMessage(QtWarningMsg, "Online info for unknown mod ghost");

        mStore->updateOnline(unknown, withNewerRelease());

        QCOMPARE(mStore->modsCount(), 0);
    }

    void updateOnlineLayersTheApiInfoOnTheInstalledMod() {
        setComponents();
        addMod(modsDir(), modId(), u"1.0.0"_s);
        QStringList updated;
        connect(mStore.get(), &vsmm::ModStore::modUpdated, this,
                [&updated](QStringView id) { updated.append(id.toString()); });
        QSignalSpy changed{mStore.get(), &vsmm::ModStore::modsChanged};

        mStore->updateOnline(modId(), withNewerRelease());

        const vsmm::ModEntry *mod = mStore->find(modId());
        QVERIFY(mod->hasUpdate());
        QCOMPARE(str(mod->getLatestVersion().mVersion), u"1.1.0"_s);
        // the online name replaces the one read out of the zip
        QCOMPARE(mod->getName(), u"Carry On"_s);
        QCOMPARE(mStore->modUpdatesCount(), 1);
        QCOMPARE(mStore->property("updatesCount").toInt(), 1);
        QCOMPARE(updated, QStringList{modId()});
        QCOMPARE(changed.count(), 1);
    }

    // the trap: an unknown game version reads as 0.1.0, nothing matches it and every mod looks current
    void unknownGameVersionSilentlyDisablesUpdateDetection() {
        mGameMngr->setGameVersion(std::nullopt);
        setComponents();
        addMod(modsDir(), modId(), u"1.0.0"_s);

        mStore->updateOnline(modId(), withNewerRelease());

        QVERIFY(!mStore->find(modId())->hasUpdate());
        QCOMPARE(mStore->modUpdatesCount(), 0);
    }

    void prereleasesCountAsUpdatesOnlyWhenTheConfigSaysSo_data() {
        QTest::addColumn<bool>("includePrerelease");
        QTest::addColumn<bool>("expectedUpdate");

        QTest::newRow("prerelease-included") << true << true;
        QTest::newRow("prerelease-excluded") << false << false;
    }

    void prereleasesCountAsUpdatesOnlyWhenTheConfigSaysSo() {
        QFETCH(const bool, includePrerelease);
        QFETCH(const bool, expectedUpdate);

        vsmm::GeneralSettings general;
        general.includeModPrerelease = includePrerelease;
        mConfig->setGeneral(general);
        setComponents();
        addMod(modsDir(), modId(), u"1.0.0"_s);

        mStore->updateOnline(modId(), modJson({release(u"1.1.0-rc.1"_s, {u"1.22.0"_s})}));

        QCOMPARE(mStore->find(modId())->hasUpdate(), expectedUpdate);
    }

    // the mods dirs changing is what drives a rescan, the store drops everything it holds
    void modsDirChangeReloadsTheStore() {
        setComponents();
        addMod(modsDir(), modId(), u"1.0.0"_s);
        QTest::ignoreMessage(QtInfoMsg, "Mods reloaded: 1 installed, 0 with updates");
        mStore->onModsReloaded();
        QTRY_VERIFY(!mStore->isWorkPending());
        QSignalSpy reloading{mStore.get(), &vsmm::ModStore::modsReloading};
        QSignalSpy work{mStore.get(), &vsmm::ModStore::workChanged};

        QTest::ignoreMessage(QtInfoMsg, "Reloading mods");
        mGameMngr->setModsDirs({modsDir(), stagingDir()});

        QCOMPARE(reloading.count(), 1);
        QCOMPARE(work.count(), 1);
        QCOMPARE(mStore->modsCount(), 0);
        QVERIFY(mStore->isWorkPending());
    }

    void workPendingClearsShortlyAfterTheScanFinishes() {
        setComponents();
        addMod(modsDir(), modId(), u"1.0.0"_s);
        mStore->updateOnline(modId(), withNewerRelease());
        QTest::ignoreMessage(QtInfoMsg, "Mods reloaded: 1 installed, 1 with updates");

        mStore->onModsReloaded();

        // a delayed timer clears the flag, not the call itself
        QVERIFY(mStore->isWorkPending());
        QTRY_VERIFY(!mStore->isWorkPending());
    }

    void updateForAnUnknownModIsRefused() {
        setComponents();
        int requested{0};
        connect(mStore.get(), &vsmm::ModStore::modUpdateRequested, this,
                [&requested](const vsmm::ModEntry &) { ++requested; });
        QTest::ignoreMessage(QtWarningMsg, "Update requested for unknown mod ghost");

        mStore->update(u"ghost"_s);

        QCOMPARE(requested, 0);
    }

    void updateRequestCarriesTheStoredEntryAndBlocksReloads() {
        setComponents();
        addMod(modsDir(), modId(), u"1.0.0"_s);
        mStore->updateOnline(modId(), withNewerRelease());
        QTest::ignoreMessage(QtInfoMsg, "Mods reloaded: 1 installed, 1 with updates");
        mStore->onModsReloaded();
        QTRY_VERIFY(!mStore->isWorkPending());
        QStringList requested;
        connect(mStore.get(), &vsmm::ModStore::modUpdateRequested, this,
                [&requested](const vsmm::ModEntry &mod) { requested.append(mod.getId().toString()); });

        QTest::ignoreMessage(QtInfoMsg, "Update requested for carryon@1.0.0");
        mStore->update(modId());

        QCOMPARE(requested, QStringList{modId()});
        QVERIFY(mStore->isWorkPending());
    }

    void updateAllRequestsOnlyTheOutdatedMods() {
        setComponents();
        addMod(modsDir(), modId(), u"1.0.0"_s);
        mStore->updateOnline(modId(), withNewerRelease());
        addMod(modsDir(), u"bees"_s, u"1.0.0"_s);
        mStore->updateOnline(u"bees"_s, withoutNewerRelease());
        QStringList requested;
        connect(mStore.get(), &vsmm::ModStore::modUpdateRequested, this,
                [&requested](const vsmm::ModEntry &mod) { requested.append(mod.getId().toString()); });

        QTest::ignoreMessage(QtInfoMsg, "Update requested for all 1 outdated mods");
        mStore->updateAll();

        QCOMPARE(requested, QStringList{modId()});
    }

    void updateSelectedRequestsOnlyTheMarkedOutdatedMods() {
        setComponents();
        addMod(modsDir(), modId(), u"1.0.0"_s);
        mStore->updateOnline(modId(), withNewerRelease());
        addMod(modsDir(), u"bees"_s, u"1.0.0"_s);
        mStore->updateOnline(u"bees"_s, withNewerRelease());
        QVERIFY(!mStore->property("modsSelected").toBool());
        QSignalSpy selected{mStore.get(), &vsmm::ModStore::modSelected};

        mStore->markForUpdate(modId(), true);

        QCOMPARE(selected.count(), 1);
        QVERIFY(mStore->property("modsSelected").toBool());

        QStringList requested;
        connect(mStore.get(), &vsmm::ModStore::modUpdateRequested, this,
                [&requested](const vsmm::ModEntry &mod) { requested.append(mod.getId().toString()); });

        QTest::ignoreMessage(QtInfoMsg, "Update requested for 1 selected mods");
        mStore->updateSelected();

        QCOMPARE(requested, QStringList{modId()});
    }

    // installing a mod that is already known is an update, not a second copy
    void guiInstallOfAKnownModReplacesItLikeAnUpdate() {
        // the trash path is the config's call, and a test must not depend on the host having a trash
        vsmm::GeneralSettings general;
        general.deleteOldModVersion = false;
        mConfig->setGeneral(general);
        setComponents();
        const QFileInfo installed = writeStubZip(modsDir(), u"carryon-1.0.0.zip"_s);
        mStore->add(localInfo(modId(), u"1.0.0"_s, installed), vsmm::ModStore::ModLoadType::Init);
        QStringList added;
        connect(mStore.get(), &vsmm::ModStore::modAdded, this,
                [&added](QStringView id) { added.append(id.toString()); });
        QStringList updated;
        connect(mStore.get(), &vsmm::ModStore::modUpdated, this,
                [&updated](QStringView id) { updated.append(id.toString()); });

        QTest::ignoreMessage(QtInfoMsg, "Got newer version of mod carryon@1.1.0, replacing");
        addMod(stagingDir(), modId(), u"1.1.0"_s, vsmm::ModStore::ModLoadType::GUI);

        // one entry, updated rather than added a second time
        QCOMPARE(mStore->modsCount(), 1);
        QVERIFY(added.isEmpty());
        QCOMPARE(updated, QStringList{modId()});
        const vsmm::ModEntry *mod = mStore->find(modId());
        QCOMPARE(str(mod->getVersion()), u"1.1.0"_s);
        QCOMPARE(mod->getFileInfo().absoluteFilePath(), modsDir().absoluteFilePath(u"carryon-1.1.0.zip"_s));
        QVERIFY(QFile::exists(installed.absoluteFilePath()));
    }

    void guiInstallOfAnOlderVersionThanInstalledIsIgnored() {
        setComponents();
        addMod(modsDir(), modId(), u"1.1.0"_s);
        QTest::ignoreMessage(QtWarningMsg, "Got newer or the same version of mod carryon@1.0.0, ignoring");

        addMod(stagingDir(), modId(), u"1.0.0"_s, vsmm::ModStore::ModLoadType::GUI);

        QCOMPARE(mStore->modsCount(), 1);
        QCOMPARE(str(mStore->find(modId())->getVersion()), u"1.1.0"_s);
        QVERIFY(!modsDir().exists(u"carryon-1.0.0.zip"_s));
    }

    // with the flag on the superseded zip goes to the trash and the update keeps its own name
    void updateWithDeleteOldVersionOnTrashesTheSupersededZip() {
        if (!trashWorksIn(stagingDir())) {
            QSKIP("No trash for this filesystem");
        }
        vsmm::GeneralSettings general;
        general.deleteOldModVersion = true;
        mConfig->setGeneral(general);
        setComponents();
        const QFileInfo installed{modsDir().absoluteFilePath(u"carryon-1.0.0.zip"_s)};
        QVERIFY(writeFile(installed.absoluteFilePath(), "old"));
        mStore->add(localInfo(modId(), u"1.0.0"_s, installed), vsmm::ModStore::ModLoadType::Init);
        const QString download = stagingDir().absoluteFilePath(u"carryon-1.1.0.zip"_s);
        QVERIFY(writeFile(download, "new"));

        QTest::ignoreMessage(QtInfoMsg, "Got newer version of mod carryon@1.1.0, replacing");
        mStore->add(localInfo(modId(), u"1.1.0"_s, QFileInfo{download}), vsmm::ModStore::ModLoadType::Update);

        QCOMPARE(modsDir().entryList(QStringList{u"*.zip"_s}, QDir::Files), QStringList{u"carryon-1.1.0.zip"_s});
        const vsmm::ModEntry *mod = mStore->find(modId());
        QCOMPARE(mod->getFileInfo().absoluteFilePath(), modsDir().absoluteFilePath(u"carryon-1.1.0.zip"_s));
        QCOMPARE(readFile(mod->getFileInfo().absoluteFilePath()), QByteArray{"new"});
    }

    // the generated name is temporary, trashing the old zip frees the wanted one and the update takes it
    void updateReclaimsTheWantedNameAfterTheOldZipIsTrashed() {
        if (!trashWorksIn(stagingDir())) {
            QSKIP("No trash for this filesystem");
        }
        vsmm::GeneralSettings general;
        general.deleteOldModVersion = true;
        mConfig->setGeneral(general);
        setComponents();
        const QFileInfo installed{modsDir().absoluteFilePath(u"carryon.zip"_s)};
        QVERIFY(writeFile(installed.absoluteFilePath(), "old"));
        mStore->add(localInfo(modId(), u"1.0.0"_s, installed), vsmm::ModStore::ModLoadType::Init);
        const QString download = stagingDir().absoluteFilePath(u"carryon.zip"_s);
        QVERIFY(writeFile(download, "new"));
        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Mod carryon@1.1.0 update has the same filename as %1"_s.arg(
                                               installed.absoluteFilePath())));
        QTest::ignoreMessage(QtInfoMsg, "Got newer version of mod carryon@1.1.0, replacing");

        mStore->add(localInfo(modId(), u"1.1.0"_s, QFileInfo{download}), vsmm::ModStore::ModLoadType::Update);

        QCOMPARE(modsDir().entryList(QStringList{u"*.zip"_s}, QDir::Files), QStringList{u"carryon.zip"_s});
        const vsmm::ModEntry *mod = mStore->find(modId());
        QCOMPARE(mod->getFileInfo().absoluteFilePath(), installed.absoluteFilePath());
        QCOMPARE(readFile(mod->getFileInfo().absoluteFilePath()), QByteArray{"new"});
    }

    void updateDownloadForAnUnknownModIsRefused() {
        setComponents();
        QTest::ignoreMessage(QtWarningMsg, "Update downloaded for unknown mod carryon");

        addMod(stagingDir(), modId(), u"1.1.0"_s, vsmm::ModStore::ModLoadType::Update);

        QCOMPARE(mStore->modsCount(), 0);
    }

    // the download is not trusted to be newer, the API decides that and the store checks it
    void updateDownloadThatIsNotNewerIsIgnored() {
        setComponents();
        addMod(modsDir(), modId(), u"1.1.0"_s);
        QTest::ignoreMessage(QtWarningMsg, "Got newer or the same version of mod carryon@1.0.0, ignoring");

        addMod(stagingDir(), modId(), u"1.0.0"_s, vsmm::ModStore::ModLoadType::Update);

        QCOMPARE(str(mStore->find(modId())->getVersion()), u"1.1.0"_s);
        // the stale download never reaches the mods dir
        QVERIFY(!modsDir().exists(u"carryon-1.0.0.zip"_s));
    }

    void updateInstallsTheDownloadBesideTheOldZipAndRepointsTheEntry() {
        // the trash path is the config's call, and a test must not depend on the host having a trash
        vsmm::GeneralSettings general;
        general.deleteOldModVersion = false;
        mConfig->setGeneral(general);
        setComponents();
        const QFileInfo installed = writeStubZip(modsDir(), u"carryon-1.0.0.zip"_s);
        mStore->add(localInfo(modId(), u"1.0.0"_s, installed), vsmm::ModStore::ModLoadType::Init);
        QStringList updated;
        connect(mStore.get(), &vsmm::ModStore::modUpdated, this,
                [&updated](QStringView id) { updated.append(id.toString()); });

        QTest::ignoreMessage(QtInfoMsg, "Got newer version of mod carryon@1.1.0, replacing");
        addMod(stagingDir(), modId(), u"1.1.0"_s, vsmm::ModStore::ModLoadType::Update);

        QCOMPARE(updated, QStringList{modId()});
        const vsmm::ModEntry *mod = mStore->find(modId());
        QCOMPARE(str(mod->getVersion()), u"1.1.0"_s);
        // the download lands next to the zip it supersedes, and the entry follows it
        QCOMPARE(mod->getFileInfo().absoluteFilePath(), modsDir().absoluteFilePath(u"carryon-1.1.0.zip"_s));
        QVERIFY(QFile::exists(installed.absoluteFilePath()));
    }

    // a mod whose zip name carries no version downloads onto its own file name
    void updateWithACollidingFileNameLandsUnderAUniqueName() {
        vsmm::GeneralSettings general;
        general.deleteOldModVersion = false;
        mConfig->setGeneral(general);
        setComponents();
        const QFileInfo installed = writeStubZip(modsDir(), u"carryon.zip"_s);
        mStore->add(localInfo(modId(), u"1.0.0"_s, installed), vsmm::ModStore::ModLoadType::Init);
        const QFileInfo download = writeStubZip(stagingDir(), u"carryon.zip"_s);
        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Mod carryon@1.1.0 update has the same filename as %1"_s.arg(
                                               installed.absoluteFilePath())));
        QTest::ignoreMessage(QtInfoMsg, "Got newer version of mod carryon@1.1.0, replacing");

        mStore->add(localInfo(modId(), u"1.1.0"_s, download), vsmm::ModStore::ModLoadType::Update);

        QVERIFY(mStore->contains(modId()));
        const QFileInfo stored = mStore->find(modId())->getFileInfo();
        QVERIFY(stored.exists());
        QCOMPARE(stored.absolutePath(), modsDir().absolutePath());
        QVERIFY(stored.fileName() != u"carryon.zip"_s);
        QVERIFY(stored.fileName().endsWith(u".zip"_s));
        // the old zip is untouched, deleteOldModVersion is off
        QVERIFY(QFile::exists(installed.absoluteFilePath()));
    }

    void removeAnnouncesBeforeTheEntryIsGoneAndDeletesTheZip() {
        setComponents();
        const QFileInfo zip = writeStubZip(modsDir(), u"carryon-1.0.0.zip"_s);
        mStore->add(localInfo(modId(), u"1.0.0"_s, zip), vsmm::ModStore::ModLoadType::Init);
        // the list model and the icon cache drop their rows first, so the entry must still be there
        bool presentWhileAnnounced{false};
        connect(mStore.get(), &vsmm::ModStore::modRemoved, this, [this, &presentWhileAnnounced](QStringView id) {
            presentWhileAnnounced = mStore->contains(id.toString());
        });
        QSignalSpy changed{mStore.get(), &vsmm::ModStore::modsChanged};

        QTest::ignoreMessage(QtInfoMsg, "Mod carryon deleted");
        mStore->remove(modId());

        QVERIFY(presentWhileAnnounced);
        QVERIFY(!mStore->contains(modId()));
        QCOMPARE(mStore->modsCount(), 0);
        QCOMPARE(changed.count(), 1);
        QVERIFY(!QFile::exists(zip.absoluteFilePath()));
    }

    // the row stays rather than pointing at a file the store failed to delete
    void removeKeepsTheModWhenTheZipCannotBeDeleted() {
        setComponents();
        const QFileInfo zip = writeStubZip(modsDir(), u"carryon-1.0.0.zip"_s);
        mStore->add(localInfo(modId(), u"1.0.0"_s, zip), vsmm::ModStore::ModLoadType::Init);
        QVERIFY(QFile::remove(zip.absoluteFilePath()));
        int announced{0};
        connect(mStore.get(), &vsmm::ModStore::modRemoved, this, [&announced](QStringView) { ++announced; });

        QTest::ignoreMessage(QtCriticalMsg, qPrintable(u"Failed to delete %1"_s.arg(zip.absoluteFilePath())));
        mStore->remove(modId());

        QCOMPARE(announced, 0);
        QVERIFY(mStore->contains(modId()));
        QCOMPARE(mStore->modsCount(), 1);
    }

    void removalOfAnUnknownModIsRefused() {
        setComponents();
        QTest::ignoreMessage(QtWarningMsg, "Removal requested for unknown mod ghost");

        mStore->remove(u"ghost"_s);

        QCOMPARE(mStore->modsCount(), 0);
    }
};

QTEST_GUILESS_MAIN(ModStoreUnitTest)
#include "ModStoreUnitTest.moc"
