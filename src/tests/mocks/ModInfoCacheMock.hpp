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

#include <IModInfoCache.hpp>
#include <ModEntry.hpp>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>
#include <utility>

namespace vsmm {
// keeps details in memory, the list is never parsed, tests send modListStored themselves
class ModInfoCacheMock final : public IModInfoCache {
  public:
    struct SetCall {
        QString mId;
        ModOnlineInfo mInfo;
    };

    [[nodiscard]] std::optional<ModOnlineInfo> getModInfo(const QString &modId) const override {
        mGetCalls.append(modId);
        const auto it = mStored.constFind(modId);
        if (it == mStored.constEnd()) {
            return std::nullopt;
        }
        return it.value();
    }
    void setModInfo(const QString &modId, const ModOnlineInfo &modInfo) override {
        mSetCalls.append({modId, modInfo});
        mStored.insert(modId, modInfo);
    }
    void storeModList(QByteArray modList) override { mStoredLists.append(std::move(modList)); }

    // details a previous session left behind, getModInfo hits for them
    void store(const QString &modId, ModOnlineInfo modInfo) { mStored.insert(modId, std::move(modInfo)); }
    // what ModInfoCache sends once its worker is done with a list
    void emitListStored(bool stored) { emit modListStored(stored); }

    [[nodiscard]] const QStringList &getCalls() const { return mGetCalls; }
    [[nodiscard]] const QList<SetCall> &setCalls() const { return mSetCalls; }
    [[nodiscard]] const QList<QByteArray> &storedLists() const { return mStoredLists; }

  private:
    QHash<QString, ModOnlineInfo> mStored;
    // getModInfo is const in the interface, the calls are still worth checking
    mutable QStringList mGetCalls;
    QList<SetCall> mSetCalls;
    QList<QByteArray> mStoredLists;
};
} // namespace vsmm
