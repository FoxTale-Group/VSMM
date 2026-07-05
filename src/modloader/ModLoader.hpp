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

#include <ModEntry.hpp>
#include <ModImageProvider.hpp>
#include <ModStore.hpp>

#include <Config.hpp>
#include <HttpClient.hpp>
#include <ModLoaderExport.hpp>

#include <QDir>
#include <QThreadPool>
#include <qqmlintegration.h>

namespace vsmm {
class MODLOADER_EXPORT ModLoader : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    static constexpr QLatin1StringView DOWNLOAD_CONTENT_TYPE{"application/zip"};
    static constexpr QLatin1StringView ONLINE_CONTENT_TYPE{"application/json"};
    static constexpr QLatin1StringView ONLINE_JSON_ROOT_KEY{"mod"};
    static constexpr QLatin1StringView ONLINE_LOGOFILE_JSON_KEY{"logofile"};
    static constexpr QLatin1StringView ONLINE_STATUSCODE_JSON_KEY{"statuscode"};
    static constexpr QLatin1StringView LOCAL_JSON_VERSION_KEY{"version"};
    static constexpr QLatin1StringView LOCAL_JSON_MODID_KEY{"modid"};
    static constexpr QLatin1StringView LOCAL_JSON_AUTHORS_KEY{"authors"};
    static constexpr QLatin1StringView LOCAL_JSON_NAME_KEY{"name"};

  public:
    ModLoader();
    bool initModsList();
    void setHttpClient(HttpClient *httpClient);
    void setConfig(Config *config);
    void setStore(ModStore *store);
    void load(QFileInfo &&fileInfo);

  signals:
    void modIconDownloaded(const QString &modId, QImage image); // used by imgprovider
    void allModsReloaded();                                     // used by modstore

  public slots:
    void onModsReloading();
    void onLoadFromGUI(const QUrl &filePath);
    void onModUpdateRequested(const ModEntry &mod);

  private:
    void load_(QFileInfo &&fileInfo);

    void onModInfoRetrieved(QString modId, QByteArray data);
    void onModIconRetrieved(QString modId, QByteArray data);
    void onModUpdateRetrieved(QByteArray data, ModEntry::LatestVersion latestVersion);
    void incrementModsLoadingInProgress();
    void decrementModsLoadingInProgress();

    [[nodiscard]] static QVariant getLocalInfoFromZip(QFileInfo &&fileInfo);
    [[nodiscard]] static QVariant parseLocalJson(const QByteArray &jsonByteArray, QFileInfo &&fileInfo);
    [[nodiscard]] static QJsonObject createOnlineModEntry(QByteArray jsonByteArray, QAnyStringView modId);

    Config *mConfig{nullptr};
    ModStore *mStore{nullptr};
    HttpClient *mHttpClient{nullptr};
    QAtomicInteger<quint32> mModsLoadingInProgress{0};
    QThreadPool mThreadPoolExtractZips{this};
    QThreadPool mThreadPoolProcessIcon{this};
    QThreadPool mThreadPoolProcessUpdate{this};
};
} // namespace vsmm
