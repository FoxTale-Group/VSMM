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

namespace vsmodchecker {
    ModStore::ModStore(QObject *parent) : QObject{parent} {
    }

    const ModEntry& ModStore::add(ModEntry mod) {
        const QString id = mod.getId().toString();
        const auto it = mMods.insert(id, std::move(mod));
        emit modAdded(*it);
        emit modsModified();
        return *it;
    }

    const ModEntry& ModStore::replace(ModEntry mod) {
        const QString id = mod.getId().toString();
        const auto it = mMods.insert(id, std::move(mod));
        emit modUpdated(*it);
        emit modsModified();
        return *it;
    }

    void ModStore::clear() {
        if (mMods.isEmpty()) {
            return;
        }
        mMods.clear();
        emit cleared();
        emit modsModified();
    }

    bool ModStore::contains(const QString &id) const {
        return mMods.contains(id);
    }

    const ModEntry* ModStore::find(const QString &id) const {
        const auto it = mMods.constFind(id);
        return it == mMods.constEnd() ? nullptr : &it.value();
    }

    int ModStore::count() const {
        return static_cast<int>(mMods.size());
    }

    int ModStore::updates() const {
        int updates{0};
        for (const auto &mod : mMods) {
            if (mod.hasUpdate()) {
                updates++;
            }
        }
        return updates;
    }
} // vsmodchecker
