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

Q_STATIC_LOGGING_CATEGORY(cImageProvider, "imageprovider");

namespace vsmm {
ModImageProvider::ModImageProvider() : QQuickImageProvider(Image) {}

QImage ModImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    qCDebug(cImageProvider, "Image requested for %s at %dx%d", qUtf8Printable(id), requestedSize.width(),
            requestedSize.height());
    QImage image;
    {
        QMutexLocker locker(&mMutex);
        QString key = id.section('?', 0, 0);
        image = mImages.value(key);
    }
    if (size)
        *size = image.size();
    if (requestedSize.isValid() && !image.isNull()) {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        qCDebug(cImageProvider, "Image for %s scaled to %dx%d", qUtf8Printable(id), requestedSize.width(),
                requestedSize.height());
    }

    return image;
}

qint64 ModImageProvider::getCacheKey(QStringView id) const {
    QMutexLocker locker(&mMutex);
    const auto it = mImages.constFind(id);
    if (it == mImages.constEnd()) {
        return 0;
    }
    const qint64 cacheKey = it->cacheKey();
    qCDebug(cImageProvider, "Cache key for %s is %lld", qUtf8Printable(id.toString()), cacheKey);
    return cacheKey;
}

bool ModImageProvider::hasImage(QStringView id) const {
    QMutexLocker locker(&mMutex);
    const bool hasImage = mImages.contains(id);
    qCDebug(cImageProvider, "Image for %s present: %s", qUtf8Printable(id.toString()), hasImage ? "true" : "false");
    return hasImage;
}

void ModImageProvider::onImageReceived(QStringView modId, QImage image) {
    {
        QMutexLocker locker(&mMutex);
        if (const auto it = mImages.find(modId); it != mImages.end()) {
            *it = std::move(image);
            qCDebug(cImageProvider, "Image for %s updated", qUtf8Printable(modId.toString()));
        } else {
            mImages.insert(modId.toString(), std::move(image));
            qCDebug(cImageProvider, "Image for %s added", qUtf8Printable(modId.toString()));
        }
    }
    emit imageAdded(modId);
}

void ModImageProvider::onModsReloading() {
    QMutexLocker locker(&mMutex);
    qCDebug(cImageProvider, "Clearing all images");
    mImages.clear();
}

void ModImageProvider::onModRemoved(QStringView modId) {
    QMutexLocker locker(&mMutex);
    qCDebug(cImageProvider, "Removing image for %s", qUtf8Printable(modId.toString()));
    mImages.removeIf([modId](const QPair<QString, QImage> &entry) { return entry.first == modId.toString(); });
}
} // namespace vsmm