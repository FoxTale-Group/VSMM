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
#include <QList>

#include <utility>

namespace vsmm {
// emits in the same order as ModStore, the entry is always findable while its signal runs
class ModStoreMock final : public IModStore {
  public:
    struct AddCall {
        ModLocalInfo mInfo;
        ModLoadType mLoadType;
    };
    struct OnlineCall {
        QString mId;
        QJsonObject mOnlineInfo;
    };
    struct ProgressCall {
        QString mId;
        qint64 mReceived;
        qint64 mTotal;
    };
    struct FailedCall {
        QString mId;
        QString mReason;

        bool operator==(const FailedCall &) const = default;
    };

    // only keeps what it received, the loader tests check it
    void add(ModLocalInfo localModInfo, ModLoadType loadType) override {
        mAddCalls.append({std::move(localModInfo), loadType});
    }
    void updateOnline(QStringView id, QJsonObject onlineInfo) override {
        mOnlineCalls.append({id.toString(), std::move(onlineInfo)});
    }
    void onModsReloaded() override { ++mReloadedCount; }
    void onModUpdateProgress(const QString &modId, qint64 received, qint64 total) override {
        mProgressCalls.append({modId, received, total});
    }
    void onModUpdateFailed(QString modId, QString reason) override {
        mFailedCalls.append({std::move(modId), std::move(reason)});
    }

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

    void requestUpdate(const QStringList &modIds) { emit modUpdateRequested(modIds); }

    // what ModStore re-emits for the update model while a batch runs
    void reportProgress(const QString &id, qint64 received, qint64 total) {
        emit modUpdateProgress(id, received, total);
    }
    void reportInstalling(const QString &id) { emit modUpdateInstalling(id); }
    void reportFinished(const QString &id) { emit modUpdateFinished(id); }
    void reportFailed(const QString &id, const QString &reason) { emit modUpdateFailed(id, reason); }
    void finishLoading() { emit modsLoaded(); }

    [[nodiscard]] const QList<AddCall> &addCalls() const { return mAddCalls; }
    [[nodiscard]] const QList<OnlineCall> &onlineCalls() const { return mOnlineCalls; }
    [[nodiscard]] int reloadedCount() const { return mReloadedCount; }
    [[nodiscard]] const QList<ProgressCall> &progressCalls() const { return mProgressCalls; }
    [[nodiscard]] const QList<FailedCall> &failedCalls() const { return mFailedCalls; }

  private:
    QHash<QString, ModEntry> mMods;
    QList<AddCall> mAddCalls;
    QList<OnlineCall> mOnlineCalls;
    QList<ProgressCall> mProgressCalls;
    QList<FailedCall> mFailedCalls;
    int mReloadedCount{0};
};
} // namespace vsmm
