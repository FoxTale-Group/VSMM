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

#include <QString>
#include <ZipArchiveExport.hpp>
#include <expected>
#include <zip.h>

namespace vsmm {
class ZIPARCHIVE_EXPORT ZipArchive {
  public:
    using FileIndex = zip_int64_t;
    using FileContentSize = zip_int64_t;

    explicit ZipArchive(QString file);
    Q_DISABLE_COPY(ZipArchive)

    ZipArchive(ZipArchive &&other) noexcept;
    ZipArchive &operator=(ZipArchive &&other) noexcept;

    [[nodiscard]] std::expected<void, QString> open();
    [[nodiscard]] std::expected<FileIndex, QString> getFileIndex(QUtf8StringView fileName) const;
    [[nodiscard]] std::expected<QByteArray, QString> getFileContent(FileIndex fileIndex) const;
    ~ZipArchive();

  private:
    QString mFile;
    zip_t *mZipFile{nullptr};
};
} // namespace vsmm
