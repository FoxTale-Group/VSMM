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
#include <ModEntry.hpp>
#include <ModStoreExport.hpp>

#include <QHash>
#include <QObject>
#include <qqmlintegration.h>

namespace vsmm {
class MODSTORE_EXPORT ModStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int installedModsCount READ modsCount NOTIFY modsChanged)
    Q_PROPERTY(int updatesCount READ modUpdatesCount NOTIFY modsChanged)
    Q_PROPERTY(bool workPending READ isWorkPending NOTIFY workChanged)
    Q_PROPERTY(bool modsSelected READ modsSelected NOTIFY modSelected)

  public:
    enum class ModLoadType : std::uint8_t { Init = 0, GUI, Update };

    explicit ModStore(QObject *parent = nullptr);

    void setConfig(IConfig *config);
    void setGameMngr(IGameMngr *gameMngr);

    void add(ModEntry::LocalInfo localModInfo, ModLoadType loadType);
    void updateOnline(QStringView id, QJsonObject onlineInfo);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void update(const QString &id);
    Q_INVOKABLE void updateAll();
    Q_INVOKABLE void updateSelected();
    Q_INVOKABLE void markForUpdate(const QString &id, bool marked);
    Q_INVOKABLE void setFavorite(const QString &id, bool favorite);
    Q_INVOKABLE void remove(const QString &id);

    [[nodiscard]] bool contains(const QString &id) const;
    [[nodiscard]] const ModEntry *find(const QString &id) const;
    [[nodiscard]] int modsCount() const;
    [[nodiscard]] int modUpdatesCount() const;
    [[nodiscard]] bool isWorkPending() const;

  signals:
    void modSelected();                           // NOTIFY modsSelected — QML bindings only
    void modsChanged();                           // NOTIFY installedModsCount/updatesCount — QML bindings only
    void workChanged();                           // NOTIFY workPending — QML bindings only
    void modAdded(QStringView modId);             // used by modlistmodel
    void modUpdated(QStringView modId);           // used by modlistmodel
    void modsReloading();                         // used by modlistmodel, modloader & imgprovider
    void modUpdateRequested(const ModEntry &mod); // used by modloader
    void modRemoved(QStringView modId);           // used by modlistmodel & imgprovider

  public slots:
    void onModsReloaded();

  private slots:
    void onModsDirChanged();

  private:
    template <typename Obj, typename Signal, typename... Args>
    void emitSignal(Signal &&signal, Obj *obj, Args &&...args) {
        emit(obj->*signal)(std::forward<Args>(args)...);
        emit modsChanged();
    }

    [[nodiscard]] bool modsSelected() const;
    void addOnInit(ModEntry::LocalInfo localModInfo);
    void addFromGUI(ModEntry::LocalInfo localModInfo);
    void updateMod(ModEntry::LocalInfo localModInfo);
    static void toUniqueFileName(QString &name);

    QHash<QString, ModEntry> mMods;

    // either update happening or mods are being reloaded
    bool mWorkPending{true};

    IConfig *mConfig{nullptr};
    IGameMngr *mGameMngr{nullptr};
    QSet<QString> mFavoriteMods;
};
} // namespace vsmm
