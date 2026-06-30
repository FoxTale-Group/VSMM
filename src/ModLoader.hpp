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

#include <QDir>
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
    [[nodiscard]] bool setModsPath(const QString &modsPath);
    bool initModsList();
    void setNetworkManager(QNetworkAccessManager *networkManager);
    void setStore(ModStore *store);
    void load(const QFileInfo &fileInfo);

  signals:
    void modIconDownloaded(const QString &modId, QImage image);
    void allModsReloaded();

  public slots:
    void onModsReloading();
    void onLoadFromGUI(const QString &filePath);

  private slots:
    void notifyModProcessed();

  private:
    struct ModInfoZip {
        QString name, version, id, author, filename;
    };
    void retrieveInfoForMod(ModInfoZip info, const QString &filePath);
    void retrieveModIcon(const QString &id, const QUrl &url);
    void load(const QString &filePath, bool fromGUI);

    static ModInfoZip parseModInfoJson(QByteArrayView jsonByteArray, const QString &filename);

    ModStore *mStore{nullptr};
    QNetworkAccessManager *mNetworkManager{nullptr};
    QDir mModsPath;
    qint64 mRequestCount{0};
    QThreadPool mThreadPoolExtractZips;

  private slots:
    void requestInfoFinished();
};
} // namespace vsmodchecker
