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

#include "ZipArchive.hpp"

namespace vsmodchecker {
    ZipArchive::ZipArchive(const std::filesystem::path &path) {
        int errorCode{};
        mZipFile = zip_open(path.string().c_str(), ZIP_RDONLY, &errorCode);
        if (!mZipFile) {
            throw Exception(std::format("Failed to open zip file: {}", path));
        }
    }

    ZipArchive::ZipArchive(ZipArchive &&other) noexcept {
        if (mZipFile) {
            zip_close(mZipFile);
        }

        mZipFile = other.mZipFile;
        other.mZipFile = nullptr;
    }

    ZipArchive& ZipArchive::operator=(ZipArchive&& other) noexcept {
        if (mZipFile) {
            zip_close(mZipFile);
        }

        mZipFile = other.mZipFile;
        other.mZipFile = nullptr;
        return *this;
    }

    ZipArchive::FileIndex ZipArchive::getFileIndex(std::string_view fileName) const {
        return zip_name_locate(mZipFile, fileName.data(), ZIP_FL_ENC_UTF_8);
    }

    std::pair<std::unique_ptr<char[]>, ZipArchive::FileContentSize> ZipArchive::getFileContent(FileIndex fileIndex) const {
        zip_stat_t fileStats;
        if (const int res = zip_stat_index(mZipFile, fileIndex, ZIP_FL_ENC_UTF_8, &fileStats); res != ZIP_ER_OK) {
            throw Exception("Failed to get file stats in zip file", res);
        }

        zip_file_t *modInfoFile = zip_fopen_index(mZipFile, fileIndex, ZIP_FL_ENC_UTF_8);
        if (!modInfoFile) {
            throw Exception("Failed to open file in zip file");
        }

        auto buffer = std::make_unique<char[]>(fileStats.size);
        if (const zip_int64_t bytesRead = zip_fread(modInfoFile, buffer.get(), fileStats.size); bytesRead != fileStats.size) {
            throw Exception("Failed to read file from zip file");
        }

        return {std::move(buffer), fileStats.size};
    }

    ZipArchive::~ZipArchive() {
        zip_close(mZipFile);
        mZipFile = nullptr;
    }
} // vsmodchecker