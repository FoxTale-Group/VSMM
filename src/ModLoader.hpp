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

#include <filesystem>
#include <QNetworkAccessManager>
#include <QThreadPool>
#include <qqmlintegration.h>

namespace vsmodchecker {
    class ModLoader : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

    public:
        ModLoader() = default;
        [[nodiscard]] bool setModsPath(std::filesystem::path modsPath);
        bool initModsList();
        void setNetworkManager(QNetworkAccessManager *networkManager);
        void setModImageProvider(ModImageProvider* modImageProvider);
        void setStore(ModStore *store);

    signals:
        void thumbnailReady(const QString &modId);

    public slots:
        void onModsReloaded();

    private:
        struct ModInfoZip {
            QString name, version, id, author, filename;
        };
        void retrieveInfoForMod(ModInfoZip info);
        void retrieveModIcon(const QString& id, const QUrl& url);

        static ModInfoZip parseModInfoJson(QByteArrayView jsonByteArray, const QString &filename);

        ModStore *mStore{nullptr};
        QNetworkAccessManager *mNetworkManager{nullptr};
        std::filesystem::path mModsPath;
        qint64 mRequestCount{0};
        ModImageProvider* mModImageProvider{nullptr};
        QThreadPool mThreadPoolExtractZips;

    private slots:
        void requestInfoFinished();
    };
} // vsmodchecker
