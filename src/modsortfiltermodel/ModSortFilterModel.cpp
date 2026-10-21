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

#include "ModSortFilterModel.hpp"
#include <ModListModel.hpp>
#include <QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(cModSortFilterModel, "modsortfiltermodel");

namespace vsmm {
ModSortFilterModel::ModSortFilterModel(QObject *parent) : QSortFilterProxyModel{parent} {
    setDynamicSortFilter(true);
    QSortFilterProxyModel::sort(0);
    setSortRole(ModListModel::NameRole);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterRole(ModListModel::NameRole);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void ModSortFilterModel::setFilterText(const QString &filterText) {
    setFilterFixedString(filterText);

    qCDebug(cModSortFilterModel, "Filter set to '%s', %d of %d rows visible", qUtf8Printable(filterText), rowCount(),
            sourceModel() ? sourceModel()->rowCount() : 0);
}
} // namespace vsmm