/*
 * VS Mod Manager - A mod management tool for Vintage Story
 * Copyright (C) 2026 Amaroq & StardustVulpine
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

#include "ModListModel.hpp"

namespace vsmodchecker {
    ModListModel::ModListModel(QObject *parent) : QAbstractListModel(parent) {

    }

    int ModListModel::rowCount(const QModelIndex &parent) const {
        if (parent.isValid()) {
            return 0;
        }
        return static_cast<int>(mModsList.size());
    }

    int ModListModel::count() const {
        return static_cast<int>(mModsList.size());
    }

    QVariant ModListModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() > mModsList.size()) {
            return {};
        }

        const auto& mod = mModsList.at(index.row());
        switch (role) {
            case NameRole:
                return mod.name;
            case VersionRole:
                return mod.version;
            case AuthorRole:
                return mod.author;
            case UpdateVersionRole:
                return mod.updateVersion;
            case TagsRole:
                return mod.tags;
            default:
                return {};
        }

        return {};
    }

    QHash<int, QByteArray> ModListModel::roleNames() const {
        return {
            {NameRole, "name"},
            {VersionRole, "version"},
            {AuthorRole, "author"},
            {UpdateVersionRole, "updateVersion"},
            {TagsRole, "tags"}
        };
    }

    void ModListModel::addMod(const ModEntry &mod) {
        beginInsertRows(QModelIndex(), static_cast<int>(mModsList.size()), static_cast<int>(mModsList.size()));
        mModsList.append(mod);
        endInsertRows();
        emit countChanged();
    }

    void ModListModel::clear() {
        if (mModsList.isEmpty()) {
            return;
        }

        beginRemoveRows(QModelIndex(), 0, static_cast<int>(mModsList.size() - 1));
        mModsList.clear();
        endRemoveRows();
        emit countChanged();
    }
} // vsmodchecker