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

#include <InterfacesExport.hpp>
#include <ModEntry.hpp>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringView>

#include <cstdint>

namespace vsmm {
class INTERFACES_EXPORT IModStore : public QObject {
    Q_OBJECT

  public:
    enum class ModLoadType : std::uint8_t { Init = 0, GUI, Update };

    explicit IModStore(QObject *parent = nullptr) : QObject{parent} {}
    ~IModStore() override;

    virtual void add(ModEntry::LocalInfo localModInfo, ModLoadType loadType) = 0;
    virtual void updateOnline(QStringView id, QJsonObject onlineInfo) = 0;
    [[nodiscard]] virtual const ModEntry *find(const QString &id) const = 0;

  signals:
    void modAdded(QStringView modId);             // used by modlistmodel
    void modUpdated(QStringView modId);           // used by modlistmodel
    void modsReloading();                         // used by modlistmodel, modloader & imgprovider
    void modRemoved(QStringView modId);           // used by modlistmodel & imgprovider
    void modUpdateRequested(const ModEntry &mod); // used by modloader

  public slots:
    virtual void onModsReloaded() = 0;
};
} // namespace vsmm
