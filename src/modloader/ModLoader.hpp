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

#include <IGameMngr.hpp>
#include <IHttpClient.hpp>
#include <IModStore.hpp>
#include <ModLoaderExport.hpp>

#include <QDir>
#include <QThreadPool>
#include <qqmlintegration.h>

#include <expected>

namespace vsmm {

struct LocalInfoForeign {
    Q_GADGET
    QML_VALUE_TYPE(localModInfo)
    QML_FOREIGN(ModLocalInfo)
};

class MODLOADER_EXPORT ModLoader : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(ModLoader)
    QML_SINGLETON

    static constexpr QLatin1StringView DEFAULT_API_URL{"https://mods.vintagestory.at/api/mod/"};
    static constexpr QLatin1StringView DOWNLOAD_CONTENT_TYPE{"application/zip"};
    static constexpr QLatin1StringView ONLINE_CONTENT_TYPE{"application/json"};
    static constexpr QLatin1StringView ONLINE_JSON_ROOT_KEY{"mod"};
    static constexpr QLatin1StringView ONLINE_STATUSCODE_JSON_KEY{"statuscode"};
    static constexpr QLatin1StringView LOCAL_JSON_VERSION_KEY{"version"};
    static constexpr QLatin1StringView LOCAL_JSON_MODID_KEY{"modid"};
    static constexpr QLatin1StringView LOCAL_JSON_AUTHORS_KEY{"authors"};
    static constexpr QLatin1StringView LOCAL_JSON_NAME_KEY{"name"};
    static constexpr QLatin1StringView LOCAL_JSON_DESC_KEY{"description"};
    static constexpr QLatin1StringView LOCAL_JSON_SIDE_KEY{"side"};
    static constexpr QLatin1StringView LOCAL_JSON_DEPS_KEY{"dependencies"};

  public:
    ModLoader();
    bool initModsList();
    void setHttpClient(IHttpClient *httpClient);
    void setGameMngr(IGameMngr *gameMngr);
    void setStore(IModStore *store);
    // mod id is appended to it
    void setApiUrl(QUrl apiUrl);
    void load(QFileInfo &&fileInfo);
    Q_INVOKABLE void load(const QUrl &filePath);
    Q_INVOKABLE static ModLocalInfo readLocalInfo(const QUrl &filePath);

  signals:
    void allModsReloaded(); // used by modstore

  public slots:
    void onModsReloading();
    void onModUpdateRequested(QStringList modIds);

  private:
    // updatedModId names the requested mod for an Update load, so a broken download can be reported
    void load_(QFileInfo fileInfo, IModStore::ModLoadType modLoadType, QString updatedModId = {});
    void onModUpdateRetrieved(QByteArray data, ModEntry::LatestVersion latestVersion, QString modId);
    void reportUpdateFailed(QString modId, QString reason);
    void incrementModsLoadingInProgress(qint64 count = 1);
    void decrementModsLoadingInProgress(bool notify = true);

    [[nodiscard]] static std::expected<ModLocalInfo, QString> getLocalInfoFromZip(QFileInfo fileInfo);
    [[nodiscard]] static std::expected<ModLocalInfo, QString> parseLocalJson(const QByteArray &jsonByteArray,
                                                                             QFileInfo &&fileInfo);
    [[nodiscard]] static QJsonObject createOnlineModEntry(const QByteArray &jsonByteArray, QAnyStringView modId);

    IGameMngr *mGameMngr{nullptr};
    IModStore *mStore{nullptr};
    QUrl mApiUrl{QString{DEFAULT_API_URL}};
    IHttpClient *mHttpClient{nullptr};
    QAtomicInteger<quint32> mModsLoadingInProgress{0};
    QThreadPool mThreadPoolExtractZips{this};
    QThreadPool mThreadPoolProcessUpdate{this};
};
} // namespace vsmm
