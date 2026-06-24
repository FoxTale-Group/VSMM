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
        return static_cast<int>(mMods.size());
    }

    QVariant ModListModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() >= mMods.size()) {
            return {};
        }

        const ModEntry& mod = *std::next(mMods.begin(), index.row());
        switch (role) {
            case NameRole:
                return mod.getName().toString();
            case VersionRole:
                return mod.getVersion().toString();
            case AuthorRole:
                return mod.getAuthor().toString();
            case UpdateVersionRole:
                return mod.getUpdateVersion().toString();
            case TagsRole:
                return mod.getTags();
            case UrlRole:
                return mod.getUrl();
            case InfoReceivedRole:
                return mod.hasInfoReceived();
            case TypeRole:
                return mod.getType().toString();
            case HasUpdateRole:
                return mod.hasUpdate();
            case IconRole: {
                if (!mImageProvider || !mImageProvider->hasImage(mod.getId().toString())) {
                    return QString();
                }
                return QStringLiteral("image://modicon/%1?diff=%2").arg(mod.getId().toString()).arg(mImageProvider->getDiff(mod.getId().toString()));
            }
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
            {TagsRole, "tags"},
            {UrlRole, "url"},
            {InfoReceivedRole, "infoReceived"},
            {TypeRole, "type"},
            {HasUpdateRole, "hasUpdate"},
            {IconRole, "modicon"},
        };
    }

    void ModListModel::setModImageProvider(ModImageProvider *provider) {
        mImageProvider = provider;
    }

    void ModListModel::modEntryAdded(const ModEntry &mod) {
        const QString modId = mod.getId().toString();

        if (mIdToRow.contains(modId)) {
            return;
        }

        const int row = static_cast<int>(mMods.size());
        beginInsertRows({}, row, row);
        mMods.emplace_back(mod);
        mIdToRow.insert(modId, row);
        endInsertRows();
    }

    void ModListModel::modEntryUpdated(const ModEntry &mod) {
        const QString modId = mod.getId().toString();
        auto it = mIdToRow.find(modId);
        if (it == mIdToRow.end()) {
            return;
        }

        const int row = *it;
        mMods[row] = mod;
        if (const QModelIndex idx = index(row); idx.isValid()) {
            emit dataChanged(idx, idx);
        }
    }

    void ModListModel::modEntryIconUpdated(const QString &modId) {
        auto it = mIdToRow.find(modId);
        if (it == mIdToRow.end()) {
            return;
        }

        const int row = *it;
        if (const QModelIndex idx = index(row); idx.isValid()) {
            emit dataChanged(idx, idx, {IconRole});
        }
    }

    void ModListModel::modsCleared() {
        if (mMods.isEmpty()) {
            return;
        }
        beginRemoveRows(QModelIndex(), 0, static_cast<int>(mMods.size() - 1));
        mMods.clear();
        mIdToRow.clear();
        endRemoveRows();
    }
} // vsmodchecker