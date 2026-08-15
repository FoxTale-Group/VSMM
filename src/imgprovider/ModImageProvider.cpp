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
#include <QLoggingCategory>
#include <QQuickImageResponse>
#include <QUrlQuery>
#include <utility>

Q_STATIC_LOGGING_CATEGORY(cImageProvider, "imageprovider");

namespace {
class AsyncImageResponse : public QQuickImageResponse {
  public:
    explicit AsyncImageResponse(QString modId, const QSize requestedSize)
        : mModId{std::move(modId)}, mRequestedSize{requestedSize} {}

    void resolve(QImage image, QString error = {}) {
        if (mFinished || mCancelled) {
            qCDebug(cImageProvider, "Response for %s already resolved", qUtf8Printable(mModId));
            return;
        }

        mFinished = true;
        if (!error.isEmpty()) {
            mErrorString = std::move(error);
        } else if (mRequestedSize.isValid()) {
            image = image.scaled(mRequestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        mImage = std::move(image);
        qCDebug(cImageProvider, "Response for %s resolved", qUtf8Printable(mModId));
        emit finished();
    }

    [[nodiscard]] QString errorString() const override { return mErrorString; }

    void cancel() override {
        if (mFinished) {
            return;
        }
        qCDebug(cImageProvider, "Requested to cancel image processing for %s", qUtf8Printable(mModId));
        mCancelled = true;
        mFinished = true;
        mErrorString = QStringLiteral("Cancelled");
        emit finished();
    }

    [[nodiscard]] QQuickTextureFactory *textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(mImage);
    }

    [[nodiscard]] QStringView modId() const { return mModId; }

  private:
    QString mModId;
    QSize mRequestedSize;
    QImage mImage;
    QString mErrorString;
    bool mFinished{false};
    bool mCancelled{false};
};
} // namespace

namespace vsmm {

ModImageProvider::~ModImageProvider() { mDecodeImagesPool.waitForDone(); }

QQuickImageResponse *ModImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize) {
    QString modId = id.section(u'?', 0, 0);

    QMutexLocker locker(&mMutex);
    // check if the mod has icon
    if (mNoIcon.contains(modId)) {
        qCDebug(cImageProvider, "Returning no icon for %s", qUtf8Printable(modId));
        auto *response = new AsyncImageResponse(modId, requestedSize);
        response->resolve({}, QStringLiteral("No icon for mod"));
        return response;
    }
    // check if the mod has an icon in the cache
    if (QImage *image = mCache.object(modId); image) {
        qCDebug(cImageProvider, "Returning cached image for %s", qUtf8Printable(modId));
        auto *response = new AsyncImageResponse(modId, requestedSize);
        response->resolve(*image);
        return response;
    }

    auto *response = new AsyncImageResponse{modId, requestedSize};
    connect(this, &ModImageProvider::imageDownloaded, response,
            [response, this](const QString &downloadedId, QImage image, QString error) {
                if (downloadedId == response->modId()) {
                    if (!disconnect(response)) {
                        qCWarning(cImageProvider, "Failed to disconnect image response");
                    }
                    response->resolve(std::move(image), std::move(error));
                }
            });

    if (!mInProgress.contains(modId)) {
        mInProgress.insert(modId);
        QUrlQuery query{id.section(u'?', 1)};
        QUrl logoUrl{query.queryItemValue(QStringLiteral("url"), QUrl::FullyDecoded)};
        QMetaObject::invokeMethod(
            this,
            [this, modId, logoUrl = std::move(logoUrl)] mutable { download(std::move(modId), std::move(logoUrl)); },
            Qt::QueuedConnection);
    }
    return response;
}

void ModImageProvider::setHttpClient(HttpClient *httpClient) {
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
}

void ModImageProvider::onModRemoved(QStringView modId) {
    const QString id = modId.toString();
    QMutexLocker locker(&mMutex);
    qCDebug(cImageProvider, "Removing image for %s", qUtf8Printable(id));
    mCache.remove(id);
    mNoIcon.remove(id);
}

void ModImageProvider::download(QString modId, QUrl url) {
    if (!mHttpClient) {
        qCFatal(cImageProvider, "Http client is not set, cannot fetch icons");
    }

    mHttpClient->sendGet(
        url, this, QStringLiteral("image/"),
        [this, modId](QByteArray data) mutable {
            mDecodeImagesPool.start([this, modId = std::move(modId), data = std::move(data)] mutable {
                QImage image;
                if (!image.loadFromData(data) || image.isNull()) {
                    qCWarning(cImageProvider, "Failed to decode icon for %s: invalid image data",
                              qUtf8Printable(modId));
                    finish(std::move(modId), {}, QStringLiteral("Invalid image data"), true);
                    return;
                }
                finish(std::move(modId), std::move(image), {});
            });
        },
        [this, modId](QString error) {
            qCWarning(cImageProvider, "Failed to download icon for %s: %s", qUtf8Printable(modId),
                      qUtf8Printable(error));
            finish(modId, {}, std::move(error));
        });
}

void ModImageProvider::finish(QString modId, QImage image, QString error, bool permanentError) {
    {
        QMutexLocker locker(&mMutex);
        mInProgress.remove(modId);
        if (error.isEmpty()) {
            if (!mCache.insert(modId, new QImage(image), image.sizeInBytes())) {
                qCWarning(cImageProvider, "Failed to cache icon for %s", qUtf8Printable(modId));
            }
        } else if (permanentError) {
            mNoIcon.insert(modId);
        }
    }
    emit imageDownloaded(std::move(modId), std::move(image), std::move(error));
}
} // namespace vsmm