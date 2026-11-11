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

#include <IModInfoCache.hpp>
#include <ModInfoCacheExport.hpp>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QThreadPool>
#include <array>
#include <memory>
#include <utility>

namespace vsmm {
class MODINFOCACHE_EXPORT ModInfoCache : public IModInfoCache {
    Q_OBJECT

    static constexpr int CACHE_VERSION{1};
    static constexpr std::chrono::seconds PRUNE_AFTER{std::chrono::days{30}};
    static constexpr QLatin1StringView CONNECTION_NAME{"modinfocache"};
    static constexpr QLatin1StringView WORKER_CONNECTION_NAME{"modinfocache-worker"};

  public:
    explicit ModInfoCache(QObject *parent = nullptr);
    ~ModInfoCache() override;
    [[nodiscard]] std::optional<ModOnlineInfo> getModInfo(const QString &modId) const override;
    void setModInfo(const QString &modId, const ModOnlineInfo &modInfo) override;
    void storeModList(QByteArray modList) override;

  private:
    enum class QueryType : std::size_t {
        Details,
        Tags,
        Screenshots,
        Releases,
        ReleaseVersions,
        DeleteDetails,
        InsertDetails,
        InsertTag,
        InsertScreenshot,
        InsertRelease,
        InsertReleaseVersion,
        Count
    };
    bool resetSchema(QSqlQuery &query);
    bool createStatements();
    [[nodiscard]] const std::unique_ptr<QSqlQuery> &getQuery(QueryType queryType) const;
    std::array<std::unique_ptr<QSqlQuery>, std::to_underlying(QueryType::Count)> mStatements;
    const QString mDbLocation{QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/modinfocache.db"};
    QSqlDatabase mDatabase;
    QThreadPool mWriter;
};
} // namespace vsmm
