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
ModListModel::ModListModel(QObject *parent) : QAbstractListModel(parent) {}

int ModListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(mOrder.size());
}

QVariant ModListModel::data(const QModelIndex &index, int role) const {
    if (!mStore || !index.isValid() || index.row() < 0 || index.row() >= mOrder.size()) {
        return {};
    }

    const ModEntry *modPtr = mStore->find(mOrder.at(index.row()));
    if (!modPtr) {
        return {};
    }
    const ModEntry &mod = *modPtr;
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
        return QStringLiteral("image://modicon/%1?diff=%2")
            .arg(mod.getId().toString())
            .arg(mImageProvider->getDiff(mod.getId().toString()));
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

void ModListModel::setModImageProvider(ModImageProvider *provider) { mImageProvider = provider; }

void ModListModel::setStore(ModStore *store) {
    mStore = store;
    connect(store, &ModStore::modAdded, this, &ModListModel::onModAdded);
    connect(store, &ModStore::modUpdated, this, &ModListModel::onModUpdated);
    connect(store, &ModStore::modsReloading, this, &ModListModel::onModsReloading);
}

void ModListModel::onModAdded(const ModEntry &mod) {
    const QString modId = mod.getId().toString();
    if (mIdToRow.contains(modId)) {
        return;
    }

    const int row = static_cast<int>(mOrder.size());
    beginInsertRows({}, row, row);
    mOrder.append(modId);
    mIdToRow.insert(modId, row);
    endInsertRows();
}

void ModListModel::onModUpdated(const ModEntry &mod) {
    const auto it = mIdToRow.constFind(mod.getId().toString());
    if (it == mIdToRow.constEnd()) {
        return;
    }
    if (const QModelIndex idx = index(*it); idx.isValid()) {
        emit dataChanged(idx, idx);
    }
}

void ModListModel::iconUpdate(const QString &modId) {
    const auto it = mIdToRow.constFind(modId);
    if (it == mIdToRow.constEnd()) {
        return;
    }
    if (const QModelIndex idx = index(*it); idx.isValid()) {
        emit dataChanged(idx, idx, {IconRole});
    }
}

void ModListModel::onModsReloading() {
    if (mOrder.isEmpty()) {
        return;
    }
    beginRemoveRows(QModelIndex(), 0, static_cast<int>(mOrder.size() - 1));
    mOrder.clear();
    mIdToRow.clear();
    endRemoveRows();
}
} // namespace vsmodchecker