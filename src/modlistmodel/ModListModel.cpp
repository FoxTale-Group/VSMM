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
#include <QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(cModListModel, "modlistmodel");

namespace vsmm {
ModListModel::ModListModel(QObject *parent) : QAbstractListModel(parent) {}

int ModListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(mOrder.size());
}

QVariant ModListModel::data(const QModelIndex &index, int role) const {
    using namespace Qt::StringLiterals;
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
        const QUrl logoUrl = mod->getLogoUrl();
        // wait for online info
        if (logoUrl.isEmpty()) {
            return QString();
        }
        return u"image://modicon/%1?url=%2"_s.arg(mod->getId().toString(),
                                                  QString::fromLatin1(QUrl::toPercentEncoding(logoUrl.toString())));
    }
    case IdRole:
        return mod->getId().toString();
    case FavoriteRole:
        return mod->isFavorite();
    default:
        return {};
    }
}

QHash<int, QByteArray> ModListModel::roleNames() const {
    return {{NameRole, "modName"},          {VersionRole, "modVersion"},
            {AuthorRole, "modAuthor"},      {LatestVersionRole, "modLatestVersion"},
            {TagsRole, "modTags"},          {UrlRole, "modUrl"},
            {TypeRole, "modSide"},          {HasUpdateRole, "modHasUpdate"},
            {IconRole, "modThumbnail"},     {IdRole, "modId"},
            {FavoriteRole, "isFavoriteMod"}};
}

void ModListModel::setStore(IModStore *store) {
    if (mStore) {
        qCWarning(cModListModel, "ModStore already set");
        return;
    }
    if (!store) {
        qCFatal(cModListModel, "ModStore is null");
        return;
    }

    mStore = store;
    connect(store, &IModStore::modAdded, this, &ModListModel::onModAdded);
    connect(store, &IModStore::modUpdated, this, &ModListModel::onModUpdated);
    connect(store, &IModStore::modsReloading, this, &ModListModel::onModsReloading);
    connect(store, &IModStore::modRemoved, this, &ModListModel::onModRemoved);
}

void ModListModel::onModAdded(QStringView modId) {
    if (mIdToRow.contains(modId)) {
        qCDebug(cModListModel, "Mod %s is already in the model", qUtf8Printable(modId.toString()));
        return;
    }

    const QString modIdStr = modId.toString();
    const int row = static_cast<int>(mOrder.size());
    beginInsertRows({}, row, row);
    mOrder.append(modIdStr);
    mIdToRow.insert(modIdStr, row);
    endInsertRows();
    qCDebug(cModListModel, "Mod %s inserted at row %d", qUtf8Printable(modIdStr), row);
}

void ModListModel::onModUpdated(QStringView modId) {
    const auto it = mIdToRow.constFind(modId);
    if (it == mIdToRow.constEnd()) {
        qCDebug(cModListModel, "Update for mod %s outside the model", qUtf8Printable(modId.toString()));
        return;
    }
    if (const QModelIndex idx = index(*it); idx.isValid()) {
        emit dataChanged(idx, idx);
    }
}

void ModListModel::onModsReloading() {
    if (mOrder.isEmpty()) {
        return;
    }
    qCDebug(cModListModel, "Dropping %lld rows for reload", static_cast<long long>(mOrder.size()));
    beginRemoveRows({}, 0, static_cast<int>(mOrder.size() - 1));
    mOrder.clear();
    mIdToRow.clear();
    endRemoveRows();
}

void ModListModel::onModRemoved(QStringView modId) {
    const auto it = mIdToRow.constFind(modId);
    if (it == mIdToRow.constEnd()) {
        qCDebug(cModListModel, "Removal of mod %s outside the model", qUtf8Printable(modId.toString()));
        return;
    }

    const int row = *it;
    qCDebug(cModListModel, "Mod %s removed from row %d", qUtf8Printable(modId.toString()), row);
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