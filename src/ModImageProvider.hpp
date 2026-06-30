/*
 * VS Mod Manager - A mod management tool for Vintage Story
 * Copyright (C) 2026 Amaroq & StardustVulpine
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

#include <QHash>
#include <QMutex>
#include <QQuickImageProvider>

namespace vsmodchecker {
class ModImageProvider : public QQuickImageProvider {
    Q_OBJECT

  public:
    struct ImageEntry {
        QImage image;
        qint64 diff{0};
    };

    ModImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
    qint64 getDiff(const QString &id) const;
    bool hasImage(const QString &id) const;

  signals:
    void imageAdded(const QString &id);

  public slots:
    void onImageReceived(const QString &id, QImage image);
    void onModsReloading();

  private:
    QHash<QString, ImageEntry> mImages;
    mutable QMutex mMutex;
};
} // namespace vsmodchecker
