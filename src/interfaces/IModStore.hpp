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
#include <QObject>
#include <QString>
#include <QStringView>

namespace vsmm {
class ModEntry;

class INTERFACES_EXPORT IModStore : public QObject {
    Q_OBJECT

  public:
    explicit IModStore(QObject *parent = nullptr) : QObject{parent} {}
    ~IModStore() override;

    [[nodiscard]] virtual const ModEntry *find(const QString &id) const = 0;

  signals:
    void modAdded(QStringView modId);   // used by modlistmodel
    void modUpdated(QStringView modId); // used by modlistmodel
    void modsReloading();               // used by modlistmodel, modloader & imgprovider
    void modRemoved(QStringView modId); // used by modlistmodel & imgprovider
};
} // namespace vsmm
