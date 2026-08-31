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

#include <ModListModel.hpp>

#include <QAbstractListModel>
#include <QStringList>

namespace vsmm::test {

// stands in for ModListModel: the proxy only ever reads NameRole, so a list of names is the whole source it needs
class SourceModelStub final : public QAbstractListModel {
    Q_OBJECT

  public:
    using QAbstractListModel::QAbstractListModel;

    explicit SourceModelStub(const QStringList &names, QObject *parent = nullptr)
        : QAbstractListModel{parent}, mNames{names} {}

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex{}) const override {
        return parent.isValid() ? 0 : static_cast<int>(mNames.size());
    }

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || index.row() >= mNames.size() || role != ModListModel::NameRole) {
            return {};
        }
        return mNames.at(index.row());
    }

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override { return {{ModListModel::NameRole, "name"}}; }

    // appends at the end, so any ordering the proxy shows is its own doing and not the source's
    void append(const QString &name) {
        beginInsertRows(QModelIndex{}, static_cast<int>(mNames.size()), static_cast<int>(mNames.size()));
        mNames.append(name);
        endInsertRows();
    }

    void rename(int row, const QString &name) {
        mNames[row] = name;
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx, {ModListModel::NameRole});
    }

    void removeAt(int row) {
        beginRemoveRows(QModelIndex{}, row, row);
        mNames.removeAt(row);
        endRemoveRows();
    }

  private:
    QStringList mNames;
};

// what the view would show, top to bottom
[[nodiscard]] inline QStringList visibleNames(const QAbstractItemModel &model) {
    QStringList names;
    names.reserve(model.rowCount(QModelIndex{}));
    for (int row = 0; row < model.rowCount(QModelIndex{}); ++row) {
        names.append(model.data(model.index(row, 0), ModListModel::NameRole).toString());
    }
    return names;
}

} // namespace vsmm::test
