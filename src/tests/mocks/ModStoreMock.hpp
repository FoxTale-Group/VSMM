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

#pragma once

#include <IModStore.hpp>
#include <ModEntry.hpp>

#include <QHash>

#include <utility>

namespace vsmm {
// emits in the same order as ModStore, the entry is always findable while its signal runs
class ModStoreMock final : public IModStore {
  public:
    [[nodiscard]] const ModEntry *find(const QString &id) const override {
        const auto it = mMods.constFind(id);
        return it == mMods.constEnd() ? nullptr : &it.value();
    }

    // a known id is a replacement, ModStore announces it as an update
    void add(ModEntry mod) {
        QString id = mod.getId().toString();
        const bool replaced = mMods.contains(id);
        const auto it = mMods.insert(std::move(id), std::move(mod));
        if (replaced) {
            emit modUpdated(it->getId());
        } else {
            emit modAdded(it->getId());
        }
    }

    void update(const QString &id) {
        if (const auto it = mMods.constFind(id); it != mMods.constEnd()) {
            emit modUpdated(it->getId());
        }
    }

    // signal first, ModStore lets the model drop the row before the entry goes
    void remove(const QString &id) {
        if (const auto it = mMods.constFind(id); it != mMods.constEnd()) {
            emit modRemoved(it->getId());
            mMods.remove(id);
        }
    }

    // store empty first, same as ModStore::reload
    void reload() {
        mMods.clear();
        emit modsReloading();
    }

    // for initOnlineInfo and setFavorite, follow with update() like ModStore does
    [[nodiscard]] ModEntry &entry(const QString &id) { return mMods.find(id).value(); }

  private:
    QHash<QString, ModEntry> mMods;
};
} // namespace vsmm
