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

#include <QAbstractListModel>
#include <qqmlintegration.h>

namespace vsmodchecker {
    class ModListModel : public QAbstractListModel {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(int count READ count NOTIFY countChanged)

    public:
        enum Roles {
            NameRole = Qt::UserRole + 1,
            AuthorRole,
            VersionRole,
            UpdateVersionRole,
            TagsRole,
            UrlRole,
            InfoReceivedRole,
            TypeRole,
            HasUpdateRole,
            IconRole
        };

        explicit ModListModel(QObject *parent = nullptr);
        [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
        [[nodiscard]] int count() const;
        [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
        [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
        void setModImageProvider(ModImageProvider *provider);

    signals:
        void countChanged();

    public slots:
        void modEntryAdded(const ModEntry& mod);
        void modEntryUpdated(const ModEntry& mod);
        void modEntryIconUpdated(const QString &modId);
        void modsCleared();

    private:
        QMap<QString, ModEntry> mModsMap; // For displaying
        QMap<QString, std::reference_wrapper<ModEntry>> mModsIdMap;
        ModImageProvider *mImageProvider;
    };
} // vsmodchecker
