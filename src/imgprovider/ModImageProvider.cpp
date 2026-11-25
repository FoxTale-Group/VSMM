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

#include "ModImageProvider.hpp"

#include "AsyncImageResponse.hpp"
#include "ZipArchive.hpp"

#include <QLoggingCategory>
#include <QQuickImageResponse>
#include <QUrlQuery>
#include <utility>

Q_STATIC_LOGGING_CATEGORY(cImageProvider, "imageprovider");

namespace vsmm {

ModImageProvider::~ModImageProvider() { mDecodeImagesPool.waitForDone(); }

QQuickImageResponse *ModImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize) {
    const QString modId = id.section('?', 0, 0);
    auto *response = new imgprovider::AsyncImageResponse{id, requestedSize};

    QMutexLocker locker(&mMutex);
    if (mNoIcon.contains(id)) {
        qCDebug(cImageProvider, "Returning no icon for %s", qUtf8Printable(modId));
        response->resolve({}, QStringLiteral("No icon for mod %1").arg(modId));
        return response;
    }
    // check if the mod has an icon in the cache
    if (const QImage *image = mCache.object(id); image) {
        qCDebug(cImageProvider, "Returning cached image for %s", qUtf8Printable(modId));
        response->resolve(*image);
        return response;
    }

    connect(this, &ModImageProvider::imageDownloaded, response,
            [response, this](const QString &downloadedId, QImage image, QString error) {
                if (downloadedId == response->id()) {
                    if (!disconnect(response)) {
                        qCWarning(cImageProvider, "Failed to disconnect image response");
                    }
                    response->resolve(std::move(image), std::move(error));
                }
            });

    if (!mInProgress.contains(id)) {
        mInProgress.insert(id);
        mDecodeImagesPool.start([this, id] {
            const QUrlQuery query{id.section(u'?', 1)};
            QString filePath = query.queryItemValue(QStringLiteral("file"), QUrl::FullyDecoded);
            QUrl logoUrl{query.queryItemValue(QStringLiteral("url"), QUrl::FullyDecoded)};

            getModIconFromFileOrDownload(id, std::move(filePath), std::move(logoUrl));
        });
    }
    return response;
}

void ModImageProvider::setHttpClient(IHttpClient *httpClient) {
    if (mHttpClient) {
        qCWarning(cImageProvider, "Http client already set");
        return;
    }
    if (!httpClient) {
        qCFatal(cImageProvider, "Http client is null");
    }

    mHttpClient = httpClient;
}

void ModImageProvider::onModsReloading() {
    QMutexLocker locker(&mMutex);
    qCDebug(cImageProvider, "Clearing all images");
    mNoIcon.clear();
    mCache.clear();
    mNoLocalIcon.clear();
}

void ModImageProvider::onModRemoved(QStringView modId) {
    const auto isKeyOfModId = [modId](const QString &key) {
        return key.section(u'?', 0, 0).section(u'@', 0, 0) == modId;
    };

    QMutexLocker locker(&mMutex);
    qCDebug(cImageProvider, "Removing images for %s", qUtf8Printable(modId.toString()));
    for (const QString &key : mCache.keys()) {
        if (isKeyOfModId(key)) {
            mCache.remove(key);
        }
    }
    mNoIcon.removeIf(isKeyOfModId);
    mNoLocalIcon.removeIf(isKeyOfModId);
}

void ModImageProvider::download(QString id, QUrl url) {
    if (!mHttpClient) {
        qCFatal(cImageProvider, "Http client is not set, cannot fetch icons");
    }

    const QString modId = id.section(u'?', 0, 0);
    mHttpClient->sendGet(
        url, this, QStringLiteral("image/"),
        [this, id, modId](QByteArray data) mutable {
            mDecodeImagesPool.start([this, id = std::move(id), modId, data = std::move(data)] mutable {
                QImage image;
                if (!image.loadFromData(data)) {
                    qCWarning(cImageProvider, "Failed to decode icon for %s: invalid image data",
                              qUtf8Printable(modId));
                    finish(std::move(id), {}, QStringLiteral("Invalid image data"), true);
                    return;
                }
                finish(std::move(id), std::move(image));
            });
        },
        [this, id, modId](QString error) mutable {
            qCWarning(cImageProvider, "Failed to download icon for %s: %s", qUtf8Printable(modId),
                      qUtf8Printable(error));
            finish(std::move(id), {}, std::move(error));
        },
        [modId](const long long received, const long long total) {
            if (total < 0) {
                return;
            }
            qCDebug(cImageProvider, "Downloaded modicon for %s (%lld of %lld)", qUtf8Printable(modId), received, total);
        });
}

void ModImageProvider::finish(QString id, QImage image, QString error, const bool permanentError) {
    {
        QMutexLocker locker(&mMutex);
        mInProgress.remove(id);
        if (error.isEmpty()) {
            if (!mCache.insert(id, new QImage(image), image.sizeInBytes())) {
                qCWarning(cImageProvider, "Failed to cache icon for %s", qUtf8Printable(id.section(u'?', 0, 0)));
            }
        } else if (permanentError) {
            mNoIcon.insert(id);
        }
    }
    emit imageDownloaded(std::move(id), std::move(image), std::move(error));
}

void ModImageProvider::getModIconFromFileOrDownload(QString id, QString filePath, QUrl logoUrl) {
    const QString modId = id.section(u'?', 0, 0);
    auto downloadOnline = [this, id, logoUrl = std::move(logoUrl), modId]() mutable {
        if (logoUrl.isEmpty()) {
            finish(id, {}, QStringLiteral("No icon url for mod %1").arg(modId));
            return;
        }
        QMetaObject::invokeMethod(
            this,
            [this, id = std::move(id), logoUrl = std::move(logoUrl)] mutable {
                download(std::move(id), std::move(logoUrl));
            },
            Qt::QueuedConnection);
    };

    QMutexLocker locker{&mMutex};

    auto addNoLocalIcon = [this, &locker, &id] {
        locker.relock();
        mNoLocalIcon.insert(id);
        locker.unlock();
    };

    const bool noLocalModIcon = mNoLocalIcon.contains(id);
    locker.unlock();

    if (noLocalModIcon) {
        addNoLocalIcon();
        return downloadOnline();
    }

    // Modicon in modfile is optional, if none is found or cant be decoded online image will be used instead
    ZipArchive zipArchive{std::move(filePath)};
    if (const auto result = zipArchive.open(); !result) {
        qCWarning(cImageProvider, "Failed to open zip archive %s: %s", qUtf8Printable(zipArchive.getFile().toString()),
                  qUtf8Printable(result.error()));
        addNoLocalIcon();
        return downloadOnline();
    }
    if (const auto imageId = zipArchive.getFileIndex("modicon.png"); imageId) {
        const auto fileIconBuffer = zipArchive.getFileContent(*imageId);
        if (!fileIconBuffer) {
            qCWarning(cImageProvider, "Failed to retrieve modicon for %s", qUtf8Printable(modId));
            addNoLocalIcon();
            return downloadOnline();
        }
        QImage image;
        if (!image.loadFromData(*fileIconBuffer)) {
            qCWarning(cImageProvider, "Failed to decode icon for %s: invalid image data", qUtf8Printable(modId));
            addNoLocalIcon();
            return downloadOnline();
        }

        qCDebug(cImageProvider, "Successfully decoded modicon for %s", qUtf8Printable(modId));
        finish(std::move(id), std::move(image));
        return;
    }

    qCWarning(cImageProvider, "Modicon not found in zip archive %s", qUtf8Printable(zipArchive.getFile().toString()));
    addNoLocalIcon();
    return downloadOnline();
}
} // namespace vsmm