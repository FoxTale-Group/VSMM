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

#pragma once

#include "ModEntry.hpp"
#include "ModImageProvider.hpp"
#include "ModStore.hpp"

#include <Config.hpp>

#include <QDir>
#include <QNetworkAccessManager>
#include <QThreadPool>
#include <qqmlintegration.h>

namespace vsmm {
class ModLoader : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

  public:
    ModLoader() = default;
    bool initModsList();
    void setNetworkManager(QNetworkAccessManager *networkManager);
    void setConfig(Config *config);
    void setStore(ModStore *store);
    void load(QFileInfo &&fileInfo);

  signals:
    void modIconDownloaded(const QString &modId, QImage image);
    void allModsReloaded();

  public slots:
    void onModsReloading();
    void onLoadFromGUI(const QUrl &filePath);

  private slots:
    void notifyModProcessed();

  private:
    void retrieveInfoForMod(QString modId);
    void retrieveModIcon(const QString &id, const QUrl &url);
    void load_(QFileInfo &&fileInfo);

    static QNetworkRequest createRequest(const QUrl &url);
    static LocalModInfo parseLocalJson(const QByteArray &jsonByteArray, QFileInfo &&fileInfo);
    static QJsonObject createOnlineModEntry(const QByteArray &jsonByteArray, QAnyStringView modId);

    Config *mConfig{nullptr};
    ModStore *mStore{nullptr};
    QNetworkAccessManager *mNetworkManager{nullptr};
    qint64 mRequestCount{0};
    QThreadPool mThreadPoolExtractZips;

  private slots:
    void requestInfoFinished();
};
} // namespace vsmm
