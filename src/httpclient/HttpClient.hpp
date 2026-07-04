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

#include <HttpClientExport.hpp>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QThreadPool>

namespace vsmm {
class HTTPCLIENT_EXPORT HttpClient : public QObject {
    Q_OBJECT

    static constexpr auto MAX_RETRIES{3u};
    static constexpr qint64 MAX_CACHE_SIZE = 1024 * 1024 * 200LL;
    static constexpr std::chrono::milliseconds BASE_BACKOFF{500};
    static constexpr std::chrono::milliseconds MAX_BACKOFF{10'000};

  public:
    using SuccessFn = std::function<void(QByteArray)>;
    using FailedFn = std::function<void(const QString &)>;

    explicit HttpClient(QObject *parent = nullptr);
    void sendGet(const QUrl &url, QObject *context, const QString &contentType, SuccessFn successFn,
                 FailedFn failedFn = nullptr);

  private:
    void sendGetImpl(const QUrl &url, QObject *context, const QString &contentType, SuccessFn successFn,
                     FailedFn failedFn = nullptr, uint retryCount = 0);
    [[nodiscard]] static bool shouldRetry(const QNetworkReply *reply, const QString &contentType, uint retryCount);
    [[nodiscard]] static std::chrono::milliseconds backoffDelay(const QNetworkReply *reply, uint retryCount);

    QNetworkAccessManager mNetworkManager;
    QNetworkDiskCache mNetworkDiskCache;
    QThreadPool mThreadPoolRequestRetries;
};
} // namespace vsmm
