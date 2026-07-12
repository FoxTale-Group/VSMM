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

namespace vsmm {
ModSortFilterModel::ModSortFilterModel(QObject *parent) : QSortFilterProxyModel{parent} {
    setDynamicSortFilter(true);
    QSortFilterProxyModel::sort(0);
}

bool ModSortFilterModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const {
    QString leftName = sourceModel()->data(sourceLeft, ModListModel::NameRole).toString();
    QString rightName = sourceModel()->data(sourceRight, ModListModel::NameRole).toString();

    return leftName.compare(rightName, Qt::CaseInsensitive) < 0;
}

bool ModSortFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (mFilterText.isEmpty()) {
        return true;
    }

    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    QString name = sourceModel()->data(idx, ModListModel::NameRole).toString();
    return name.contains(mFilterText, Qt::CaseInsensitive);
}

QString ModSortFilterModel::getFilterText() const { return mFilterText; }

void ModSortFilterModel::setFilterText(const QString &filterText) {
    if (mFilterText == filterText) {
        return;
    }

    beginFilterChange();
    mFilterText = filterText;
    endFilterChange();
    emit filterTextChanged();
}
} // namespace vsmm