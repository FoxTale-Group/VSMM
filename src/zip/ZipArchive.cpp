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

#include "ZipArchive.hpp"
#include <QDebug>
#include <QtSwap>
#include <utility>

namespace vsmm {
ZipArchive::ZipArchive(QString file) : mFile(std::move(file)) {}

QPair<bool, int> ZipArchive::open() {
    if (mZipFile) {
        return {true, ZIP_ER_OK};
    }

    int errorCode{-1};
    mZipFile = zip_open(mFile.toStdString().c_str(), ZIP_RDONLY, &errorCode);
    if (!mZipFile) {
        return {false, errorCode};
    }
    return {true, ZIP_ER_OK};
}

ZipArchive::ZipArchive(ZipArchive &&other) noexcept {
    if (mZipFile) {
        zip_close(mZipFile);
        mZipFile = nullptr;
    }
    qSwap(mFile, other.mFile);
}

ZipArchive &ZipArchive::operator=(ZipArchive &&other) noexcept {
    if (mZipFile) {
        zip_close(mZipFile);
        mZipFile = nullptr;
    }

    qSwap(mFile, other.mFile);
    return *this;
}

ZipArchive::FileIndex ZipArchive::getFileIndex(QUtf8StringView fileName) const {
    return zip_name_locate(mZipFile, fileName.data(), ZIP_FL_ENC_UTF_8);
}

QByteArray ZipArchive::getFileContent(FileIndex fileIndex) const {
    using namespace Qt::StringLiterals;
    zip_stat_t fileStats;
    if (const int res = zip_stat_index(mZipFile, fileIndex, ZIP_FL_ENC_UTF_8, &fileStats); res != ZIP_ER_OK) {
        qWarning() << u"Failed to stat file in zip archive: %1 {%2}"_s.arg(mFile).arg(res);
        return {};
    }

    zip_file_t *modInfoFile = zip_fopen_index(mZipFile, fileIndex, ZIP_FL_ENC_UTF_8);
    if (!modInfoFile) {
        return {};
    }

    QByteArray buffer{static_cast<qsizetype>(fileStats.size), Qt::Uninitialized};
    if (const zip_int64_t bytesRead = zip_fread(modInfoFile, buffer.data(), fileStats.size);
        bytesRead != fileStats.size) {
        zip_fclose(modInfoFile);
        return {};
    }

    zip_fclose(modInfoFile);
    return buffer;
}

ZipArchive::~ZipArchive() {
    zip_close(mZipFile);
    mZipFile = nullptr;
}
} // namespace vsmm