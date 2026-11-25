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

#include <QImage>
#include <QQuickImageResponse>
#include <QSize>
#include <QString>

namespace vsmm::imgprovider {

class AsyncImageResponse : public QQuickImageResponse {
  public:
    explicit AsyncImageResponse(QString id, QSize requestedSize);

    void resolve(QImage image, QString error = {});
    [[nodiscard]] QString errorString() const override;
    void cancel() override;
    [[nodiscard]] QQuickTextureFactory *textureFactory() const override;
    [[nodiscard]] QStringView id() const { return mId; }

  private:
    QString mId;
    QSize mRequestedSize;
    QImage mImage;
    QString mErrorString;
    bool mFinished{false};
};
} // namespace vsmm::imgprovider
