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

#include <filesystem>
#include <QNetworkAccessManager>
#include <qqmlintegration.h>

namespace vsmodchecker {
    class ModManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

    public:
        ModManager() = default;
        void setModsPath(std::filesystem::path modsPath);
        bool initModsList();
        [[nodiscard]] const QHash<QString, ModEntry>& getModsList() const;
        void setNetworkManager(QNetworkAccessManager *networkManager);
        void setModImageProvider(ModImageProvider* modImageProvider);
        Q_INVOKABLE [[nodiscard]] int updatesAvailable() const;

    private:
        struct ModInfoZip {
            QString name, version, id, author, filename;
        };
        void retrieveInfoForMod(ModInfoZip info);
        void retrieveModIcon(const QString& id, const QUrl& url);

        static ModInfoZip parseModInfoJson(QByteArrayView jsonByteArray, const QString &filename);

        QHash<QString, ModEntry> mModsList;
        QNetworkAccessManager *mNetworkManager{nullptr};
        std::filesystem::path mModsPath;
        qint64 mRequestCount{0};
        ModImageProvider* mModImageProvider{nullptr};

    signals:
        void modEntryAdded(const ModEntry& mod);
        void modEntryUpdated(const ModEntry& mod);
        void modsCleared();
        void thumbnailReady(const QString& mod);

    public slots:
        void reloadMods();

    private slots:
        void requestInfoFinished();
    };
} // vsmodchecker
