//
// Created by karol on 6/24/26.
//

#include "ModImageProvider.hpp"

namespace vsmodchecker {
    ModImageProvider::ModImageProvider() : QQuickImageProvider(Image) {}

    QImage ModImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
    {
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

    void ModImageProvider::onModsReloaded() {
        QMutexLocker locker(&mMutex);
        mImages.clear();
    }
} // vsmodchecker