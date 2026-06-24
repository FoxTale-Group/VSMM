//
// Created by karol on 6/24/26.
//

#include "ModImageProvider.hpp"

namespace vsmodchecker {
    ModImageProvider::ModImageProvider() : QQuickImageProvider(Image) {}

    QImage ModImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
    {
        QMutexLocker locker(&mMutex);
        QString key = id.section('?', 0, 0);
        QImage image = mImages.value(key).image;
        if (size)
            *size = image.size();
        if (requestedSize.isValid() && !image.isNull())
            image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        qDebug() << "Requested image for " << id << " with size " << requestedSize << " and diff " << mImages.value(key).diff;

        return image;
    }

    void ModImageProvider::addImage(const QString &id, const QImage &image)
    {
        if (hasImage(id)) {
            QMutexLocker locker(&mMutex);
            ImageEntry& entry = mImages[id];
            entry.image = image;
            entry.diff++;
            return;
        }
        QMutexLocker locker(&mMutex);
        mImages.insert(id, {.image = image});
    }

    qint64 ModImageProvider::getDiff(const QString &id) const {
        QMutexLocker locker(&mMutex);
        return mImages.value(id).diff;
    }

    bool ModImageProvider::hasImage(const QString &id) const
    {
        QMutexLocker locker(&mMutex);
        return mImages.contains(id);
    }

    void ModImageProvider::modsCleared() {
        QMutexLocker locker(&mMutex);
        mImages.clear();
    }
} // vsmodchecker