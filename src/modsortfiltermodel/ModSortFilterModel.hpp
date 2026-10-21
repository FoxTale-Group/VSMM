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

#include <ModSortFilterModelExport.hpp>
#include <QSortFilterProxyModel>
#include <qqmlintegration.h>

namespace vsmm {
class MODSORTFILTERMODEL_EXPORT ModSortFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(ModSortFilterModel)
    QML_SINGLETON

  public:
    explicit ModSortFilterModel(QObject *parent = nullptr);
    Q_INVOKABLE void setFilterText(const QString &filterText);
};
} // namespace vsmm
