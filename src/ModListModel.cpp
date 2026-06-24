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
        return static_cast<int>(mModsMap.size());
    }

    int ModListModel::count() const {
        return static_cast<int>(mModsMap.size());
    }

    QVariant ModListModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() >= mModsMap.size()) {
            return {};
        }

        const ModEntry& mod = *std::next(mModsMap.begin(), index.row());
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
        if (mModsMap.contains(mod.getName().toString())) {
            return;
        }

        int row = 0;
        for (auto it = mModsMap.constBegin(); it != mModsMap.constEnd(); ++it, ++row) {
            if (it.key() > mod.getName())
                break;
        }

        beginInsertRows(QModelIndex(), row, row);
        const auto it = mModsMap.insert(mod.getName().toString(), mod);
        mModsIdMap.insert(mod.getId().toString(), std::ref(*it));
        endInsertRows();
        emit countChanged();
    }

    void ModListModel::modEntryUpdated(const ModEntry &mod) {
        if (!mModsMap.contains(mod.getName().toString())) {
            return;
        }

        int row = 0;
        for (auto it = mModsMap.begin(); it != mModsMap.end(); ++it, ++row) {
            if (it.key() == mod.getName()) {
                *it = mod;
                mModsIdMap.insert(mod.getId().toString(), std::ref(*it));

                QModelIndex idx = index(row);
                if (idx.isValid()) {
                    emit dataChanged(idx, idx);
                }
                break;
            }
        }
    }

    void ModListModel::modEntryIconUpdated(const QString &modId) {
        int row = 0;
        for (auto it = mModsMap.begin(); it != mModsMap.end(); ++it, ++row) {
            if (it.value().getId() == modId) {
                if (QModelIndex idx = index(row); idx.isValid()) {
                    emit dataChanged(idx, idx, {IconRole});
                }
                break;
            }
        }
    }

    void ModListModel::modsCleared() {
        if (mModsMap.isEmpty()) {
            return;
        }
        beginRemoveRows(QModelIndex(), 0, static_cast<int>(mModsMap.size() - 1));
        mModsMap.clear();
        endRemoveRows();

        mModsIdMap.clear();

        emit countChanged();
    }
} // vsmodchecker