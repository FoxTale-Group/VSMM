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
void ModStore::setConfig(Config *config) { mConfig = config; }

void ModStore::add(ModEntry::LocalInfo localModInfo) {
    using namespace Qt::StringLiterals;

    if (localModInfo.mId.isEmpty()) {
        return;
    }

    if (!mConfig) {
        qFatal() << "Config is not set.";
    }

    // Check if there is already a mod with the same id
    for (auto it = mMods.begin(); it != mMods.end(); ++it) {
        if (it.key() != localModInfo.mId) {
            continue;
        }

        auto &currentMod = *it;
        if (currentMod.getVersion() < localModInfo.mVersion) {
            qInfo() << u"Got newer version of mod %1. Replacing..."_s.arg(localModInfo);

            QString modPath = currentMod.getFileInfo().absolutePath();
            QString newModFilePath = modPath + QDir::separator() + localModInfo.mFileInfo.fileName();
            // remove old mod and copy new one
            auto removeOldVersion = mConfig->getGeneral<bool>(Config::DELETE_OLD_VERSION_JSON_KEY);
            if (removeOldVersion && !QFile::moveToTrash(currentMod.getFileInfo().absoluteFilePath())) {
                qWarning() << u"Failed to move mod %1 to trash."_s.arg(currentMod);
            }

            // Dont copy mod if it's already in the mods folder
            if (!modPath.startsWith(localModInfo.mFileInfo.absolutePath()) &&
                !QFile::copy(localModInfo.mFileInfo.absoluteFilePath(), newModFilePath)) {
                qWarning() << u"Failed to copy mod %1 to %2."_s.arg(localModInfo).arg(newModFilePath);
                if (qsizetype extPos = newModFilePath.indexOf(".zip"_L1); extPos != -1) {
                    newModFilePath.insert(extPos, QString::fromStdString("_" + localModInfo.mVersion.str()));

                    // Last try to copy file to mods folder with suffixed version
                    if (!QFile::copy(localModInfo.mFileInfo.absoluteFilePath(), newModFilePath)) {
                        qWarning() << u"Failed to copy mod %1 to %2."_s.arg(localModInfo).arg(newModFilePath);
                        return;
                    }
                }
            }

            // update file info for newly copied mod
            localModInfo.mFileInfo = QFileInfo{newModFilePath};
            currentMod = ModEntry{std::move(localModInfo)};

            emitSignal(&ModStore::modUpdated, this, currentMod);
            return;
        }

        qInfo() << u"Mod %1 already has newer version in mods folder %2."_s.arg(localModInfo).arg(currentMod);
        return;
    }

    const QString id = localModInfo.mId;
    const auto it = mMods.emplace(id, std::move(localModInfo));
    emitSignal(&ModStore::modAdded, this, *it);
}

void ModStore::updateOnline(QStringView id, QJsonObject onlineInfo) {
    auto it = mMods.find(id);
    if (it == mMods.end()) {
        return;
    }
    auto &mod = *it;
    mod.initOnlineInfo(std::move(onlineInfo));
    emitSignal(&ModStore::modUpdated, this, mod);
}

void ModStore::reload() {
    if (mMods.isEmpty()) {
        return;
    }
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
    const auto it = mMods.find(id);
    if (it != mMods.end()) {
        it->setMarkedForUpdate(marked);
    }
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
} // namespace vsmm
