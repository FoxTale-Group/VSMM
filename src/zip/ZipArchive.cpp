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
#include <QtSwap>
#include <utility>

using namespace Qt::StringLiterals;

namespace {
QString errorText(zip_error_t *error) {
    return u"%1 (%2)"_s.arg(QString::fromUtf8(zip_error_strerror(error))).arg(zip_error_code_zip(error));
}

QString errorTextForCode(int errorCode) {
    zip_error_t error;
    zip_error_init_with_code(&error, errorCode);
    const QString text = errorText(&error);
    zip_error_fini(&error);
    return text;
}
} // namespace

namespace vsmm {
ZipArchive::ZipArchive(QString file) : mFile(std::move(file)) {}

std::expected<void, QString> ZipArchive::open() {
    if (mZipFile) {
        return {};
    }

    int errorCode{ZIP_ER_OK};
    mZipFile = zip_open(mFile.toStdString().c_str(), ZIP_RDONLY, &errorCode);
    if (!mZipFile) {
        return std::unexpected(u"Failed to open archive %1: %2"_s.arg(mFile, errorTextForCode(errorCode)));
    }

    return {};
}

ZipArchive::ZipArchive(ZipArchive &&other) noexcept
    : mFile(std::move(other.mFile)), mZipFile(std::exchange(other.mZipFile, nullptr)) {}

ZipArchive &ZipArchive::operator=(ZipArchive &&other) noexcept {
    qSwap(mFile, other.mFile);
    qSwap(mZipFile, other.mZipFile);
    return *this;
}

std::expected<ZipArchive::FileIndex, QString> ZipArchive::getFileIndex(QUtf8StringView fileName) const {
    if (!mZipFile) {
        return std::unexpected(u"Archive %1 is not open"_s.arg(mFile));
    }

    const QByteArray name{fileName.data(), fileName.size()};
    const FileIndex index = zip_name_locate(mZipFile, name.constData(), ZIP_FL_ENC_UTF_8);
    if (index == -1) {
        return std::unexpected(u"Failed to locate entry %1 in archive %2: %3"_s.arg(
            QString::fromUtf8(name), mFile, errorText(zip_get_error(mZipFile))));
    }

    return index;
}

std::expected<QByteArray, QString> ZipArchive::getFileContent(FileIndex fileIndex) const {
    if (!mZipFile) {
        return std::unexpected(u"Archive %1 is not open"_s.arg(mFile));
    }

    zip_stat_t fileStats;
    if (zip_stat_index(mZipFile, fileIndex, ZIP_FL_ENC_UTF_8, &fileStats) == -1) {
        return std::unexpected(u"Failed to stat entry %1 in archive %2: %3"_s.arg(fileIndex).arg(
            mFile, errorText(zip_get_error(mZipFile))));
    }

    if ((fileStats.valid & ZIP_STAT_SIZE) == 0) {
        return std::unexpected(u"Entry %1 in archive %2 has no known size"_s.arg(fileIndex).arg(mFile));
    }

    zip_file_t *entry = zip_fopen_index(mZipFile, fileIndex, ZIP_FL_ENC_UTF_8);
    if (!entry) {
        return std::unexpected(u"Failed to open entry %1 in archive %2: %3"_s.arg(fileIndex).arg(
            mFile, errorText(zip_get_error(mZipFile))));
    }

    QByteArray buffer{static_cast<qsizetype>(fileStats.size), Qt::Uninitialized};
    if (const zip_int64_t bytesRead = zip_fread(entry, buffer.data(), fileStats.size);
        bytesRead != static_cast<zip_int64_t>(fileStats.size)) {
        QString error = u"Short read of entry %1 in archive %2: %3 of %4 bytes: %5"_s.arg(fileIndex)
                            .arg(mFile)
                            .arg(bytesRead)
                            .arg(fileStats.size)
                            .arg(errorText(zip_file_get_error(entry)));
        zip_fclose(entry);
        return std::unexpected(std::move(error));
    }

    zip_fclose(entry);
    return buffer;
}

ZipArchive::~ZipArchive() {
    if (mZipFile) {
        zip_close(mZipFile);
    }
}
} // namespace vsmm
