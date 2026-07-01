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

#pragma once

#include "ModEntry.hpp"
#include "ModImageProvider.hpp"
#include "ModStore.hpp"

#include <QAbstractListModel>
#include <QHash>
#include <QList>

namespace vsmodchecker {
class ModListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

  public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        AuthorRole,
        VersionRole,
        LatestVersionRole,
        TagsRole,
        UrlRole,
        TypeRole,
        HasUpdateRole,
        IconRole
    };

    explicit ModListModel(QObject *parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setModImageProvider(ModImageProvider *provider);
    void setStore(ModStore *store);

  public slots:
    void iconUpdate(const QString &modId);

  private slots:
    void onModAdded(const ModEntry &mod);
    void onModUpdated(const ModEntry &mod);
    void onModsReloading();

  private:
    ModStore *mStore{nullptr};
    QList<QString> mOrder;        // row order -> mod id
    QHash<QString, int> mIdToRow; // mod id -> row index
    ModImageProvider *mImageProvider{nullptr};
};
} // namespace vsmodchecker
