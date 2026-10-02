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

#include <QByteArray>
#include <QByteArrayList>
#include <QList>
#include <QString>
#include <zip.h>

namespace vsmm::test {
struct Entry {
    QByteArray mName;
    QByteArray mContent;
};

// fixtures are built with libzip itself, so no binary archive has to be checked in
[[nodiscard]] inline bool writeArchive(const QString &file, const QList<Entry> &entries,
                                       const QByteArrayList &dirs = {}) {
    int errorCode{-1};
    zip_t *archive = zip_open(file.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
    if (!archive) {
        return false;
    }

    for (const auto &[name, content] : entries) {
        // the source borrows the buffer, entries outlives zip_close as the caller's argument
        zip_source_t *source = zip_source_buffer(archive, content.constData(), content.size(), 0);
        if (!source || zip_file_add(archive, name.constData(), source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) < 0) {
            zip_source_free(source);
            zip_discard(archive);
            return false;
        }
    }

    for (const auto &dir : dirs) {
        if (zip_dir_add(archive, dir.constData(), ZIP_FL_ENC_UTF_8) < 0) {
            zip_discard(archive);
            return false;
        }
    }

    return zip_close(archive) == 0;
}

// an encrypted entry stats fine but cannot be opened without the password, which no other fixture reaches.
// libzip can be built without crypto, so the caller has to treat a false here as "not testable on this build"
[[nodiscard]] inline bool writeEncryptedArchive(const QString &file, const Entry &entry) {
    int errorCode{-1};
    zip_t *archive = zip_open(file.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
    if (!archive) {
        return false;
    }

    zip_source_t *source = zip_source_buffer(archive, entry.mContent.constData(), entry.mContent.size(), 0);
    const zip_int64_t index =
        source ? zip_file_add(archive, entry.mName.constData(), source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) : -1;
    if (index < 0) {
        zip_source_free(source);
        zip_discard(archive);
        return false;
    }
    if (zip_file_set_encryption(archive, index, ZIP_EM_AES_256, "password") < 0) {
        zip_discard(archive);
        return false;
    }
    return zip_close(archive) == 0;
}
} // namespace vsmm::test
