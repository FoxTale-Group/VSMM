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
#include <QTimer>
#include <chrono>

namespace vsmm {
ModStore::ModStore(QObject *parent) : QObject{parent} {}
void ModStore::setConfig(Config *config) {
    mConfig = config;

    // Get favorites from cfg
    for (auto &modId : mConfig->getFavorites()) {
        mFavoriteMods.insert(std::move(modId));
    }
}
void ModStore::setGameMngr(GameMngr *gameMngr) {
    mGameMngr = gameMngr;
    connect(mGameMngr, &GameMngr::modsDirsChanged, this, &ModStore::onModsDirChanged);
}

void ModStore::add(ModEntry::LocalInfo localModInfo) {
    using namespace Qt::StringLiterals;

    if (localModInfo.mId.isEmpty()) {
        return;
    }

    if (!mConfig) {
        qFatal() << "Config is not set.";
    }

    if (!mGameMngr) {
        qFatal() << "GameMngr is not set.";
    }

    // Check if there is already a mod with the same id
    if (const auto it = mMods.find(localModInfo.mId); it != mMods.end()) {
        if (it->getVersion() >= localModInfo.mVersion) {
            qInfo() << u"Mod %1 already has newer version in mods folder %2."_s.arg(localModInfo)
                           .arg(it->getFileInfo().absolutePath());
            return;
        }
        qInfo() << u"Got newer version of mod %1. Replacing..."_s.arg(localModInfo);

        QString modPath = it->getFileInfo().absolutePath();
        QString newModFilePath = modPath + QDir::separator() + localModInfo.mFileInfo.fileName();
        // remove old mod and copy new one
        auto removeOldVersion = mConfig->getGeneral<bool>(Config::DELETE_OLD_VERSION_JSON_KEY);
        if (removeOldVersion && !QFile::moveToTrash(it->getFileInfo().absoluteFilePath())) {
            qWarning() << u"Failed to move mod %1 to trash."_s.arg(*it);
        }

        // Dont copy mod if it's already in the mods folder
        const bool alreadyInModsFolder = modPath.startsWith(localModInfo.mFileInfo.absolutePath());
        if (!alreadyInModsFolder && !QFile::copy(localModInfo.mFileInfo.absoluteFilePath(), newModFilePath)) {
            qWarning() << u"Failed to copy mod %1 to %2."_s.arg(localModInfo).arg(newModFilePath);

            const qsizetype extPos = newModFilePath.indexOf(".zip"_L1);
            if (extPos == -1) {
                return;
            }
            newModFilePath.insert(extPos, QString::fromStdString("_" + localModInfo.mVersion.to_string()));

            // Last try to copy file to mods folder with suffixed version
            if (!QFile::copy(localModInfo.mFileInfo.absoluteFilePath(), newModFilePath)) {
                qWarning() << u"Failed to copy mod %1 to %2."_s.arg(localModInfo).arg(newModFilePath);
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

        emitSignal(&ModStore::modUpdated, this, it->getId());
        return;
    }

    if (mGameMngr->getModsDirs().isEmpty()) {
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
    emitSignal(&ModStore::modAdded, this, it->getId());
}

void ModStore::updateOnline(QStringView id, QJsonObject onlineInfo) {
    const auto it = mMods.find(id);
    if (it == mMods.end()) {
        return;
    }
    const auto includePrerelease = mConfig->getGeneral<bool>(Config::INCLUDE_MOD_PRERELEASE_JSON_KEY);
    it->initOnlineInfo(std::move(onlineInfo),
                       mGameMngr->getGameVersion() ? *mGameMngr->getGameVersion() : semver::version{},
                       includePrerelease);
    emitSignal(&ModStore::modUpdated, this, it->getId());
}

void ModStore::reload() {
    mWorkPending = true;
    mMods.clear();
    emit workChanged();
    emitSignal(&ModStore::modsReloading, this);
}

void ModStore::load(const QUrl &filePath) { emit modAddedFromGUI(filePath); }

void ModStore::update(const QString &id) {
    const auto it = mMods.constFind(id);
    if (it == mMods.end()) {
        return;
    }

    // dont allow mods reload while update is happening
    mWorkPending = true;
    emit workChanged();
    emit modUpdateRequested(*it);
}

void ModStore::updateAll() {
    for (const auto &mod : mMods) {
        if (mod.hasUpdate()) {
            if (!mWorkPending) {
                mWorkPending = true;
                emit workChanged();
            }
            emit modUpdateRequested(mod);
        }
    }
}

void ModStore::updateSelected() {
    for (const auto &mod : mMods) {
        if (mod.hasUpdate() && mod.isMarkedForUpdate()) {
            if (!mWorkPending) {
                mWorkPending = true;
                emit workChanged();
            }
            emit modUpdateRequested(mod);
        }
    }
}

void ModStore::markForUpdate(const QString &id, bool marked) {
    if (const auto it = mMods.find(id); it != mMods.end()) {
        it->setMarkedForUpdate(marked);
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
        mFavoriteMods.removeIf([&id](const auto &val) { return val == id; });
    }
    it->setFavorite(favorite);
    mConfig->setFavorites(mFavoriteMods.values());
    emit modUpdated(it->getId());
}

void ModStore::remove(const QString &id) {
    using namespace Qt::StringLiterals;

    const auto it = mMods.find(id);
    if (it == mMods.end()) {
        return;
    }

    if (!QFile::remove(it->getFileInfo().absoluteFilePath())) {
        qCritical() << u"Failed to delete %1"_s.arg(id);
        return;
    }

    // let modlistmodel & imgprovider remove their entries first
    emit modRemoved(it->getId());

    mMods.erase(it);
    // update count of installed mods
    emit modsChanged();

    qInfo() << u"Mod %1 deleted"_s.arg(id);
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
    qDebug() << "Mods reloaded";
}

void ModStore::onModsDirChanged() { reload(); }
bool ModStore::modsSelected() const {
    return std::ranges::any_of(mMods.begin(), mMods.end(), [](const auto &mod) { return mod.isMarkedForUpdate(); });
}
} // namespace vsmm
