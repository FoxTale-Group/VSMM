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

#include "AsyncImageResponse.hpp"

#include <QLoggingCategory>
#include <QQuickTextureFactory>
#include <utility>

// same name as the provider's category, so one logging rule covers both
Q_STATIC_LOGGING_CATEGORY(cImageProvider, "imageprovider");

namespace vsmm::imgprovider {

AsyncImageResponse::AsyncImageResponse(QString id, const QSize requestedSize)
    : mId{std::move(id)}, mRequestedSize{requestedSize} {}

void AsyncImageResponse::resolve(QImage image, QString error) {
    if (mFinished) {
        qCDebug(cImageProvider, "Response for %s already resolved", qUtf8Printable(mId));
        return;
    }

    mFinished = true;
    if (!error.isEmpty()) {
        mErrorString = std::move(error);
    } else if (mRequestedSize.isValid()) {
        Qt::AspectRatioMode aspectRatio = Qt::KeepAspectRatio;
        if (mRequestedSize.height() < 100 && mRequestedSize.width() < 100) {
            aspectRatio = Qt::IgnoreAspectRatio;
        }
        image = image.scaled(mRequestedSize, aspectRatio, Qt::SmoothTransformation);
    }

    mImage = std::move(image);
    qCDebug(cImageProvider, "Response for %s resolved", qUtf8Printable(mId));
    emit finished();
}

QString AsyncImageResponse::errorString() const { return mErrorString; }

void AsyncImageResponse::cancel() {
    if (mFinished) {
        return;
    }
    qCDebug(cImageProvider, "Requested to cancel image processing for %s", qUtf8Printable(mId));
    mFinished = true;
    mErrorString = QStringLiteral("Cancelled");
    emit finished();
}

QQuickTextureFactory *AsyncImageResponse::textureFactory() const {
    return QQuickTextureFactory::textureFactoryForImage(mImage);
}
} // namespace vsmm::imgprovider
