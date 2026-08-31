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

#include "HttpClient.hpp"
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QPointer>
#include <QStandardPaths>
#include <QTimer>
#include <constants.hpp>

Q_STATIC_LOGGING_CATEGORY(cHttpClient, "httpclient");

namespace vsmm {
using namespace Qt::StringLiterals;

HttpClient::HttpClient(QObject *parent) : QObject{parent} {
    mNetworkDiskCache.setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    mNetworkDiskCache.setMaximumCacheSize(MAX_CACHE_SIZE);
    mNetworkManager.setCache(&mNetworkDiskCache);
}

void HttpClient::sendGet(const QUrl &url, QObject *context, const QString &contentType, SuccessFn successFn,
                         FailedFn failedFn) {
    sendGetImpl(url, context, contentType, std::move(successFn), std::move(failedFn), 0);
}

void HttpClient::sendGetImpl(const QUrl &url, QObject *context, const QString &contentType, SuccessFn successFn,
                             FailedFn failedFn, uint retryCount) {
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork);
    request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader,
                      QStringLiteral("%1/%2").arg(APP_NAME).arg(APP_VERSION));
    request.setTransferTimeout(std::chrono::seconds{15});

    // If retry use always network
    if (retryCount > 0) {
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    }

    qCDebug(cHttpClient, "GET %s (attempt %u)", qUtf8Printable(url.toString()), retryCount + 1);

    QNetworkReply *reply = mNetworkManager.get(request);
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    connect(reply, &QNetworkReply::finished, context,
            [this, context, reply, url, contentType, onSuccess = std::move(successFn), onFailed = std::move(failedFn),
             retryCount] mutable {
                if (shouldRetry(reply, retryCount)) {
                    const std::chrono::milliseconds delay = backoffDelay(reply, retryCount);
                    qCDebug(cHttpClient, "Retrying %s in %lld ms", qUtf8Printable(url.toString()),
                            static_cast<long long>(delay.count()));
                    QTimer::singleShot(delay, context,
                                       [self = QPointer{this}, url, contentType, context, retryCount,
                                        onSuccess = std::move(onSuccess), onFailed = std::move(onFailed)]() mutable {
                                           if (!self) {
                                               return;
                                           }
                                           self->sendGetImpl(url, context, contentType, std::move(onSuccess),
                                                             std::move(onFailed), retryCount + 1);
                                       });
                    return;
                }

                if (reply->error() != QNetworkReply::NoError ||
                    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
                    // the caller reports this with its own context, this only pins the url
                    qCDebug(cHttpClient, "GET %s failed: %s (HTTP %d)", qUtf8Printable(url.toString()),
                            qUtf8Printable(reply->errorString()),
                            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
                    if (onFailed) {
                        onFailed(u"Error receiving response. Error: %1 | HTTP Status: %2"_s.arg(reply->errorString())
                                     .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
                    }
                    return;
                }

                if (const QString received = reply->header(QNetworkRequest::ContentTypeHeader).toString();
                    !received.startsWith(contentType)) {
                    qCDebug(cHttpClient, "GET %s returned content type %s, expected %s", qUtf8Printable(url.toString()),
                            qUtf8Printable(received), qUtf8Printable(contentType));
                    if (onFailed) {
                        onFailed(u"Received data with incorrect content type."_s);
                    }
                    return;
                }

                QByteArray data = reply->readAll();
                qCDebug(cHttpClient, "GET %s returned %lld bytes", qUtf8Printable(url.toString()),
                        static_cast<long long>(data.size()));
                onSuccess(std::move(data));
            });
}

bool HttpClient::shouldRetry(const QNetworkReply *reply, uint retryCount) {
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    auto isPermanentStatus = [](const int code) {
        return (code >= 400 && code < 500 && code != 429 && code != 408) || code == 501;
    };

    if (statusCode != 200 && retryCount < MAX_RETRIES &&
        ((statusCode >= 500 && statusCode <= 504 && statusCode != 501) || statusCode == 429 || statusCode == 408)) {
        qCWarning(cHttpClient, "Failed to retrieve data: HTTP status code %d. Retrying...", statusCode);
        return true;
    }

    if (isPermanentStatus(statusCode)) {
        return false;
    }

    if (reply->error() != QNetworkReply::NoError && retryCount < MAX_RETRIES) {
        qCWarning(cHttpClient, "Failed to retrieve data: %s. Retrying...", qUtf8Printable(reply->errorString()));
        return true;
    }

    return false;
}

std::chrono::milliseconds HttpClient::backoffDelay(const QNetworkReply *reply, uint retryCount) {
    if (const QByteArray retryAfter = reply->rawHeader("Retry-After"); !retryAfter.isEmpty()) {
        bool ok = false;
        if (const int secs = retryAfter.toInt(&ok); ok && secs >= 0) {
            return std::min<std::chrono::milliseconds>(std::chrono::seconds{secs}, MAX_BACKOFF);
        }
    }

    return std::min<std::chrono::milliseconds>(BASE_BACKOFF * (1u << retryCount), MAX_BACKOFF);
}
} // namespace vsmm