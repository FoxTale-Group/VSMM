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

#include <IConfig.hpp>
#include <IGameMngr.hpp>
#include <IModStore.hpp>
#include <ModEntry.hpp>
#include <ModStoreExport.hpp>

#include <QHash>
#include <QObject>
#include <QTimer>
#include <qqmlintegration.h>

namespace vsmm {
class MODSTORE_EXPORT ModStore : public IModStore {
    Q_OBJECT
    QML_NAMED_ELEMENT(ModStore)
    QML_SINGLETON
    Q_PROPERTY(int installedModsCount READ modsCount NOTIFY modsChanged)
    Q_PROPERTY(int updatesCount READ modUpdatesCount NOTIFY modsChanged)
    Q_PROPERTY(bool workPending READ isWorkPending NOTIFY workChanged)
    Q_PROPERTY(bool modsSelected READ modsSelected NOTIFY modSelected)
    Q_PROPERTY(int enabledModsCount READ enabledModsCount NOTIFY modsChanged)

  public:
    explicit ModStore(QObject *parent = nullptr);

    void setConfig(IConfig *config);
    void setGameMngr(IGameMngr *gameMngr);

    void add(ModLocalInfo localModInfo, ModLoadType loadType) override;
    void updateOnline(QStringView id, QJsonObject onlineInfo) override;
    Q_INVOKABLE void reload();
    Q_INVOKABLE void update(const QString &id);
    Q_INVOKABLE void updateAll();
    Q_INVOKABLE void updateSelected();
    Q_INVOKABLE void markForUpdate(const QString &id, bool marked);
    Q_INVOKABLE void setFavorite(const QString &id, bool favorite);
    Q_INVOKABLE void remove(const QString &id);

    [[nodiscard]] bool contains(const QString &id) const;
    [[nodiscard]] const ModEntry *find(const QString &id) const override;
    [[nodiscard]] int modsCount() const;
    [[nodiscard]] int modUpdatesCount() const;
    [[nodiscard]] bool isWorkPending() const;
    [[nodiscard]] int enabledModsCount() const;

  signals:
    void modSelected(); // NOTIFY modsSelected - QML bindings only
    void modsChanged(); // NOTIFY installedModsCount/updatesCount/enabledModsCount - QML bindings only
    void workChanged(); // NOTIFY workPending - QML bindings only

  public slots:
    void onModsReloaded() override;
    void onModUpdateProgress(const QString &modId, qint64 received, qint64 total) override;
    void onModUpdateFailed(QString modId, QString reason) override;

  private slots:
    void onModsDirChanged();

  private:
    enum class UpdateType : quint8 { One = 0, Selected, All };

    void update(UpdateType updateType, const QString &modId = {});
    [[nodiscard]] bool modsSelected() const;
    void addOnInit(ModLocalInfo localModInfo);
    void addFromGUI(ModLocalInfo localModInfo);
    void updateMod(ModLocalInfo localModInfo);
    static void toUniqueFileName(QString &name);

    QHash<QString, ModEntry> mMods;

    // either update happening or mods are being reloaded
    bool mWorkPending{true};

    IConfig *mConfig{nullptr};
    IGameMngr *mGameMngr{nullptr};
    QSet<QString> mFavoriteMods;
    QTimer mWorkPendingTimer{this};
};
} // namespace vsmm
