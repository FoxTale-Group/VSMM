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

namespace vsmm {
ModImageProvider::ModImageProvider() : QQuickImageProvider(Image) {}

QImage ModImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    QImage image;
    {
        QMutexLocker locker(&mMutex);
        QString key = id.section('?', 0, 0);
        image = mImages.value(key).image;
    }
    if (size)
        *size = image.size();
    if (requestedSize.isValid() && !image.isNull())
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return image;
}

qint64 ModImageProvider::getDiff(const QString &id) const {
    QMutexLocker locker(&mMutex);
    return mImages.value(id).diff;
}

bool ModImageProvider::hasImage(const QString &id) const {
    QMutexLocker locker(&mMutex);
    return mImages.contains(id);
}

void ModImageProvider::onImageReceived(const QString &id, QImage image) {
    {
        QMutexLocker locker(&mMutex);
        if (const auto it = mImages.find(id); it != mImages.end()) {
            it->image = std::move(image);
            it->diff++;
        } else {
            mImages.insert(id, {.image = std::move(image)});
        }
    }
    emit imageAdded(id);
}

void ModImageProvider::onModsReloading() {
    QMutexLocker locker(&mMutex);
    mImages.clear();
}
} // namespace vsmm