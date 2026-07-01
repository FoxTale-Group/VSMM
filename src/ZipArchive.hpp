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

#include <QString>
#include <zip.h>

namespace vsmodchecker {
class ZipArchive {
  public:
    using FileIndex = zip_int64_t;
    using FileContentSize = zip_int64_t;

    explicit ZipArchive(QString file);
    ZipArchive(ZipArchive &) = delete;
    ZipArchive &operator=(ZipArchive &) = delete;

    ZipArchive(ZipArchive &&other) noexcept;
    ZipArchive &operator=(ZipArchive &&other) noexcept;

    [[nodiscard]] QPair<bool, int> open();
    [[nodiscard]] FileIndex getFileIndex(QUtf8StringView fileName) const;
    [[nodiscard]] QByteArray getFileContent(FileIndex fileIndex) const;
    ~ZipArchive();

  private:
    QString mFile;
    zip_t *mZipFile{nullptr};
};
} // namespace vsmodchecker
