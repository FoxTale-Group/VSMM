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

#include <IHttpClient.hpp>
#include <ImgProviderExport.hpp>
#include <QCache>
#include <QMutex>
#include <QQuickImageProvider>
#include <QSet>
#include <QThreadPool>

namespace vsmm {

class IMGPROVIDER_EXPORT ModImageProvider : public QQuickAsyncImageProvider {
    Q_OBJECT

  public:
    ModImageProvider() = default;
    ~ModImageProvider() override;
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
    void setHttpClient(IHttpClient *httpClient);

  signals:
    void imageDownloaded(QString id, QImage image, QString error);

  public slots:
    void onModsReloading();
    void onModRemoved(QStringView modId);

  private:
    void download(QString modId, QUrl url);
    void finish(QString modId, QImage image, QString error, bool permanentError = false);
    static constexpr qsizetype CACHE_SIZE{32 * 1024 * 1024}; // 32 MiB
    IHttpClient *mHttpClient{nullptr};
    QCache<QString, QImage> mCache{CACHE_SIZE};
    QSet<QString> mInProgress, mNoIcon;
    QThreadPool mDecodeImagesPool{this};
    mutable QMutex mMutex;
};
} // namespace vsmm
