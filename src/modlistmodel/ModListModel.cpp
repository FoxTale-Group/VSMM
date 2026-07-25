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

#include "ModListModel.hpp"

namespace vsmm {
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

    const ModEntry *mod = mStore->find(mOrder.at(index.row()));
    if (!mod) {
        return {};
    }

    switch (role) {
    case NameRole:
        return mod->getName().toString();
    case VersionRole:
        return QString::fromStdString(mod->getVersion().to_string());
    case AuthorRole:
        return mod->getAuthor().toString();
    case LatestVersionRole:
        return QString::fromStdString(mod->getLatestVersion().mVersion.to_string());
    case TagsRole:
        return mod->getTags();
    case UrlRole:
        return mod->getUrl();
    case TypeRole:
        return mod->getType().toString();
    case HasUpdateRole:
        return mod->hasUpdate();
    case IconRole: {
        if (!mImageProvider || !mImageProvider->hasImage(mod->getId().toString())) {
            return QString();
        }
        return QStringLiteral("image://modicon/%1?diff=%2")
            .arg(mod->getId().toString())
            .arg(mImageProvider->getCacheKey(mod->getId().toString()));
    }
    case IdRole:
        return mod->getId().toString();
    case FavoriteRole:
        return mod->isFavorite();
    default:
        return {};
    }

    return {};
}

QHash<int, QByteArray> ModListModel::roleNames() const {
    return {{NameRole, "modName"},          {VersionRole, "modVersion"},
            {AuthorRole, "modAuthor"},      {LatestVersionRole, "modLatestVersion"},
            {TagsRole, "modTags"},          {UrlRole, "modUrl"},
            {TypeRole, "modSide"},          {HasUpdateRole, "modHasUpdate"},
            {IconRole, "modThumbnail"},     {IdRole, "modId"},
            {FavoriteRole, "isFavoriteMod"}};
}

void ModListModel::setModImageProvider(ModImageProvider *provider) { mImageProvider = provider; }

void ModListModel::setStore(ModStore *store) {
    mStore = store;
    connect(store, &ModStore::modAdded, this, &ModListModel::onModAdded);
    connect(store, &ModStore::modUpdated, this, &ModListModel::onModUpdated);
    connect(store, &ModStore::modsReloading, this, &ModListModel::onModsReloading);
    connect(store, &ModStore::modRemoved, this, &ModListModel::onModRemoved);
}

void ModListModel::onModAdded(QStringView modId) {
    if (mIdToRow.contains(modId)) {
        return;
    }

    const QString modIdStr = modId.toString();
    const int row = static_cast<int>(mOrder.size());
    beginInsertRows({}, row, row);
    mOrder.append(modIdStr);
    mIdToRow.insert(modIdStr, row);
    endInsertRows();
}

void ModListModel::onModUpdated(QStringView modId) {
    const auto it = mIdToRow.constFind(modId);
    if (it == mIdToRow.constEnd()) {
        return;
    }
    if (const QModelIndex idx = index(*it); idx.isValid()) {
        emit dataChanged(idx, idx);
    }
}

void ModListModel::iconUpdate(QStringView modId) {
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
    beginRemoveRows({}, 0, static_cast<int>(mOrder.size() - 1));
    mOrder.clear();
    mIdToRow.clear();
    endRemoveRows();
}

void ModListModel::onModRemoved(QStringView modId) {
    const auto it = mIdToRow.constFind(modId);
    if (it == mIdToRow.constEnd()) {
        return;
    }

    const int row = *it;
    beginRemoveRows({}, row, row);
    mOrder.removeIf([modId](const QString &modId_) { return modId == modId_; });
    // Regenerate id to row
    mIdToRow.clear();
    for (qsizetype i = 0; i < mOrder.size(); ++i) {
        mIdToRow.insert(mOrder[i], static_cast<int>(i));
    }
    endRemoveRows();
}
} // namespace vsmm