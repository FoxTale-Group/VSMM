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
#include <QByteArray>
#include <QObject>
#include <QString>

#include <optional>

namespace vsmm {
class INTERFACES_EXPORT IModInfoCache : public QObject {
    Q_OBJECT

  public:
    explicit IModInfoCache(QObject *parent = nullptr) : QObject{parent} {}
    ~IModInfoCache() override;
    [[nodiscard]] virtual std::optional<ModOnlineInfo> getModInfo(const QString &modId) const = 0;
    virtual void setModInfo(const QString &modId, const ModOnlineInfo &modInfo) = 0;
    virtual void storeModList(QByteArray modList) = 0;

  signals:
    // every storeModList call, stored is false when nothing was written
    void modListStored(bool stored);
};
} // namespace vsmm
