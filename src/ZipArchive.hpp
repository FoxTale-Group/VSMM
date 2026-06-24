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

#include <filesystem>
#include <zip.h>

namespace vsmodchecker {
    class ZipArchive {
    public:
        class Exception : public std::runtime_error {
        public:
            explicit Exception(const std::string& message) : std::runtime_error(message) {}
            Exception(const std::string& message, int errCode) : std::runtime_error(message), mErrCode(errCode) {}

        private:
            int mErrCode = -1;
        };

        using FileIndex = zip_int64_t;
        using FileContentSize = zip_int64_t;

        explicit ZipArchive(const std::filesystem::path& path);
        ZipArchive(ZipArchive&) = delete("Cannot copy zip archive");
        ZipArchive& operator=(ZipArchive&) = delete("Cannot copy zip archive");

        ZipArchive(ZipArchive&& other) noexcept;
        ZipArchive& operator=(ZipArchive&& other) noexcept;

        [[nodiscard]] FileIndex getFileIndex(std::string_view fileName) const;
        [[nodiscard]] std::pair<std::unique_ptr<char[]>, FileContentSize> getFileContent(FileIndex fileIndex) const;
        ~ZipArchive();

    private:
        zip_t *mZipFile{nullptr};
    };
} // vsmodchecker
