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

#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QQuickImageResponse>
#include <QQuickTextureFactory>
#include <QTest>
#include <QUrl>

#include <memory>

namespace vsmm::test {
using namespace Qt::StringLiterals;

// a solid image encoded as png, built in memory so nothing binary is checked in
[[nodiscard]] inline QByteArray pngBytes(const QSize size = QSize{8, 8}, const QColor color = Qt::red) {
    QImage image{size, QImage::Format_ARGB32};
    image.fill(color);
    QByteArray bytes;
    QBuffer buffer{&bytes};
    // not Q_ASSERT, that would abort the run and behave differently in release builds
    QTest::qVerify(buffer.open(QIODevice::WriteOnly), "buffer.open(WriteOnly)", "", __FILE__, __LINE__);
    QTest::qVerify(image.save(&buffer, "PNG"), "image.save(PNG)", "", __FILE__, __LINE__);
    return bytes;
}

// the id ModListModel::IconRole builds, minus the image://modicon/ prefix the engine strips
[[nodiscard]] inline QString iconId(const QString &modId, const QString &logoUrl) {
    return u"%1?url=%2"_s.arg(modId, QString::fromLatin1(QUrl::toPercentEncoding(logoUrl)));
}

// what a response holds once it settled, textureFactory() hands its factory to the caller
struct Settled {
    QImage mImage;
    QString mError;
};

[[nodiscard]] inline Settled settled(const QQuickImageResponse *response) {
    const std::unique_ptr<QQuickTextureFactory> factory{response->textureFactory()};
    return {.mImage = factory ? factory->image() : QImage{}, .mError = response->errorString()};
}

// the provider hands ownership of the response to the engine, here the test owns it
using ResponsePtr = std::unique_ptr<QQuickImageResponse>;
} // namespace vsmm::test
