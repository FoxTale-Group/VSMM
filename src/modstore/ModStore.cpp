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

#include "ModStore.hpp"

#include <QDir>
#include <QLoggingCategory>
#include <QTimer>
#include <chrono>

Q_STATIC_LOGGING_CATEGORY(cModStore, "modstore");

namespace vsmm {
ModStore::ModStore(QObject *parent) : QObject{parent} {}
void ModStore::setConfig(IConfig *config) {
    if (mConfig) {
        qCWarning(cModStore, "Config already set");
        return;
    }
    if (!config) {
        qCFatal(cModStore, "Config is null");
        return;
    }

    mConfig = config;

    // Get favorites from cfg
    for (auto &modId : mConfig->getFavorites()) {
        mFavoriteMods.insert(std::move(modId));
    }

    qCDebug(cModStore, "Config set, %lld favorites loaded", static_cast<long long>(mFavoriteMods.size()));
}
void ModStore::setGameMngr(IGameMngr *gameMngr) {
    if (mGameMngr) {
        qCWarning(cModStore, "GameMngr already set");
        return;
    }
    if (!gameMngr) {
        qCFatal(cModStore, "GameMngr is null");
        return;
    }

    mGameMngr = gameMngr;
    connect(mGameMngr, &IGameMngr::modsDirsChanged, this, &ModStore::onModsDirChanged);
}

void ModStore::add(ModEntry::LocalInfo localModInfo) {
    using namespace Qt::StringLiterals;

    if (localModInfo.mId.isEmpty()) {
        qCWarning(cModStore, "Refusing mod without an id from %s",
                  qUtf8Printable(localModInfo.mFileInfo.absoluteFilePath()));
        return;
    }

    if (!mConfig) {
        qCFatal(cModStore, "Config is not set");
    }

    if (!mGameMngr) {
        qCFatal(cModStore, "GameMngr is not set");
    }

    // Check if there is already a mod with the same id
    if (const auto it = mMods.find(localModInfo.mId); it != mMods.end()) {
        if (it->getVersion() >= localModInfo.mVersion) {
            qCInfo(cModStore, "Mod %s already has newer version in mods folder %s",
                   qUtf8Printable(localModInfo.toString()), qUtf8Printable(it->getFileInfo().absolutePath()));
            return;
        }
        qCInfo(cModStore, "Got newer version of mod %s, replacing", qUtf8Printable(localModInfo.toString()));

        QString modPath = it->getFileInfo().absolutePath();
        QString newModFilePath = modPath + QDir::separator() + localModInfo.mFileInfo.fileName();
        // remove old mod and copy new one
        auto removeOldVersion = mConfig->general().deleteOldModVersion;
        if (removeOldVersion && !QFile::moveToTrash(it->getFileInfo().absoluteFilePath())) {
            qCWarning(cModStore, "Failed to move mod %s to trash", qUtf8Printable(it->toString()));
        }

        // Dont copy mod if it's already in the mods folder
        const bool alreadyInModsFolder = modPath.startsWith(localModInfo.mFileInfo.absolutePath());
        if (!alreadyInModsFolder && !QFile::copy(localModInfo.mFileInfo.absoluteFilePath(), newModFilePath)) {
            qCWarning(cModStore, "Failed to copy mod %s to %s", qUtf8Printable(localModInfo.toString()),
                      qUtf8Printable(newModFilePath));

            const qsizetype extPos = newModFilePath.indexOf(".zip"_L1);
            if (extPos == -1) {
                return;
            }
            newModFilePath.insert(extPos, QString::fromStdString("_" + localModInfo.mVersion.to_string()));

            // Last try to copy file to mods folder with suffixed version
            if (!QFile::copy(localModInfo.mFileInfo.absoluteFilePath(), newModFilePath)) {
                qCWarning(cModStore, "Failed to copy mod %s to %s", qUtf8Printable(localModInfo.toString()),
                          qUtf8Printable(newModFilePath));
                return;
            }
        }

        if (!alreadyInModsFolder) {
            // update file info for newly copied mod
            localModInfo.mFileInfo = QFileInfo{newModFilePath};
        }
        *it = ModEntry{std::move(localModInfo)};

        if (mFavoriteMods.contains(it->getId().toString())) {
            it->setFavorite(true);
        }

        qCDebug(cModStore, "Mod %s replaced", qUtf8Printable(it->toString()));
        emitSignal(&ModStore::modUpdated, this, it->getId());
        return;
    }

    if (mGameMngr->getModsDirs().isEmpty()) {
        qCWarning(cModStore, "No mods dir to add %s to", qUtf8Printable(localModInfo.toString()));
        return;
    }

    // TODO: For now add only to first dir
    QFile::copy(localModInfo.mFileInfo.absoluteFilePath(), mGameMngr->getModsDirs().first().absolutePath() +
                                                               QDir::separator() + localModInfo.mFileInfo.fileName());

    QString id = localModInfo.mId;
    const auto it = mMods.emplace(std::move(id), std::move(localModInfo));
    if (mFavoriteMods.contains(it->getId().toString())) {
        it->setFavorite(true);
    }
    qCDebug(cModStore, "Mod %s added", qUtf8Printable(it->toString()));
    emitSignal(&ModStore::modAdded, this, it->getId());
}

void ModStore::updateOnline(QStringView id, QJsonObject onlineInfo) {
    const auto it = mMods.find(id);
    if (it == mMods.end()) {
        qCWarning(cModStore, "Online info for unknown mod %s", qUtf8Printable(id.toString()));
        return;
    }
    const auto includePrerelease = mConfig->general().includeModPrerelease;
    it->initOnlineInfo(std::move(onlineInfo),
                       mGameMngr->getGameVersion() ? *mGameMngr->getGameVersion() : semver::version{},
                       includePrerelease);
    emitSignal(&ModStore::modUpdated, this, it->getId());
}

void ModStore::reload() {
    qCInfo(cModStore, "Reloading mods");
    mWorkPending = true;
    mMods.clear();
    emit workChanged();
    emitSignal(&ModStore::modsReloading, this);
}

void ModStore::load(const QUrl &filePath) {
    qCInfo(cModStore, "Adding mod from %s", qUtf8Printable(filePath.toString()));
    emit modAddedFromGUI(filePath);
}

void ModStore::update(const QString &id) {
    const auto it = mMods.constFind(id);
    if (it == mMods.end()) {
        qCWarning(cModStore, "Update requested for unknown mod %s", qUtf8Printable(id));
        return;
    }

    qCInfo(cModStore, "Update requested for %s", qUtf8Printable(it->toString()));

    // dont allow mods reload while update is happening
    mWorkPending = true;
    emit workChanged();
    emit modUpdateRequested(*it);
}

void ModStore::updateAll() {
    int requested{0};
    for (const auto &mod : mMods) {
        if (mod.hasUpdate()) {
            if (!mWorkPending) {
                mWorkPending = true;
                emit workChanged();
            }
            emit modUpdateRequested(mod);
            requested++;
        }
    }
    qCInfo(cModStore, "Update requested for all %d outdated mods", requested);
}

void ModStore::updateSelected() {
    int requested{0};
    for (const auto &mod : mMods) {
        if (mod.hasUpdate() && mod.isMarkedForUpdate()) {
            if (!mWorkPending) {
                mWorkPending = true;
                emit workChanged();
            }
            emit modUpdateRequested(mod);
            requested++;
        }
    }
    qCInfo(cModStore, "Update requested for %d selected mods", requested);
}

void ModStore::markForUpdate(const QString &id, bool marked) {
    if (const auto it = mMods.find(id); it != mMods.end()) {
        it->setMarkedForUpdate(marked);
        qCDebug(cModStore, "Mod %s marked for update: %s", qUtf8Printable(id), marked ? "true" : "false");
        emit modSelected();
    }
}

void ModStore::setFavorite(const QString &id, bool favorite) {
    const auto it = mMods.find(id);
    if (it == mMods.end()) {
        return;
    }

    if (favorite) {
        mFavoriteMods.insert(id);
    } else {
        mFavoriteMods.remove(id);
    }
    it->setFavorite(favorite);
    mConfig->setFavorites(mFavoriteMods.values());
    qCDebug(cModStore, "Mod %s favorite: %s", qUtf8Printable(id), favorite ? "true" : "false");
    emit modUpdated(it->getId());
}

void ModStore::remove(const QString &id) {
    const auto it = mMods.find(id);
    if (it == mMods.end()) {
        qCWarning(cModStore, "Removal requested for unknown mod %s", qUtf8Printable(id));
        return;
    }

    if (!QFile::remove(it->getFileInfo().absoluteFilePath())) {
        qCCritical(cModStore, "Failed to delete %s", qUtf8Printable(it->getFileInfo().absoluteFilePath()));
        return;
    }

    // let modlistmodel & imgprovider remove their entries first
    emit modRemoved(it->getId());

    mMods.erase(it);
    // update count of installed mods
    emit modsChanged();

    qCInfo(cModStore, "Mod %s deleted", qUtf8Printable(id));
}

bool ModStore::contains(const QString &id) const { return mMods.contains(id); }

const ModEntry *ModStore::find(const QString &id) const {
    const auto it = mMods.constFind(id);
    return it == mMods.constEnd() ? nullptr : &it.value();
}

int ModStore::modsCount() const { return static_cast<int>(mMods.size()); }

int ModStore::modUpdatesCount() const {
    int updates{0};
    for (const auto &mod : mMods) {
        if (mod.hasUpdate()) {
            updates++;
        }
    }
    return updates;
}
bool ModStore::isWorkPending() const { return mWorkPending; }

void ModStore::onModsReloaded() {
    constexpr std::chrono::milliseconds delay{500};
    QTimer::singleShot(delay, this, [this] {
        mWorkPending = false;
        emit workChanged();
    });
    qCInfo(cModStore, "Mods reloaded: %d installed, %d with updates", modsCount(), modUpdatesCount());
}

void ModStore::onModsDirChanged() { reload(); }
bool ModStore::modsSelected() const {
    return std::ranges::any_of(mMods.begin(), mMods.end(), [](const auto &mod) { return mod.isMarkedForUpdate(); });
}
} // namespace vsmm
