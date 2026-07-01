/*
 * VS Mod Manager - A mod management tool for Vintage Story
 * Copyright (C) 2026 Amaroq & StardustVulpine
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

namespace vsmodchecker {
ModStore::ModStore(QObject *parent) : QObject{parent} {}

void ModStore::add(LocalModInfo localModInfo) {
    using namespace Qt::StringLiterals;

    if (localModInfo.mId.isEmpty()) {
        return;
    }

    for (auto it = mMods.begin(); it != mMods.end(); ++it) {
        // Found mod with the same id, check which is newer and replace
        if (it.key() == localModInfo.mId) {
            auto &currentMod = *it;
            if (currentMod.getVersion() < localModInfo.mVersion) {
                qInfo() << u"Got newer version of mod %1. Replacing..."_s.arg(localModInfo.mId);

                QStringView modPath = currentMod.getFileInfo().absolutePath();
                // remove old mod and copy new one
                QFile::remove(currentMod.getFileInfo().absoluteFilePath());
                QFile::copy(localModInfo.mFileInfo.absoluteFilePath(),
                            modPath + QDir::separator() + localModInfo.mFileInfo.fileName());
                currentMod = ModEntry{std::move(localModInfo)};
                emitSignal(&ModStore::modUpdated, this, currentMod);
            }
            return;
        }
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
}

void ModStore::reload() {
    if (mModsBeingReloaded) {
        qWarning() << "Mods reload already in progress";
        return;
    }
    if (mMods.isEmpty()) {
        return;
    }
    mModsBeingReloaded = true;
    mMods.clear();
    emitSignal(&ModStore::modsReloading, this);
}

void ModStore::load(const QUrl &filePath) { emit modAddedFromGUI(filePath); }

bool ModStore::contains(const QString &id) const { return mMods.contains(id); }

const ModEntry *ModStore::find(const QString &id) const {
    const auto it = mMods.constFind(id);
    return it == mMods.constEnd() ? nullptr : &it.value();
}

int ModStore::count() const { return static_cast<int>(mMods.size()); }

int ModStore::updates() const {
    int updates{0};
    for (const auto &mod : mMods) {
        if (mod.hasUpdate()) {
            updates++;
        }
    }
    return updates;
}

int ModStore::reloading() const { return mModsBeingReloaded; }

void ModStore::onModsReloaded() {
    constexpr std::chrono::milliseconds delay{500};
    QTimer::singleShot(delay, this, [this] {
        mModsBeingReloaded = false;
        emit modsModified();
    });
    qDebug() << "Mods reloaded";
}
} // namespace vsmodchecker
