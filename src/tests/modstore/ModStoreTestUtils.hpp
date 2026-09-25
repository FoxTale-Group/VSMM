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

#include <ModEntryTestUtils.hpp>

#include <QDir>
#include <QFile>
#include <QTest>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

// a read-only dir does not stop a write on Windows, and root ignores the mode bits everywhere
#ifdef Q_OS_WIN
#ifdef _MSC_VER
#pragma warning(disable : 4702)
#endif
#define SKIP_WITHOUT_DIR_PERMISSIONS() QSKIP("Directory permissions do not block writes on Windows")
#else
#define SKIP_WITHOUT_DIR_PERMISSIONS()                                                                                 \
    do {                                                                                                               \
        if (geteuid() == 0) {                                                                                          \
            QSKIP("Running as root, directory permissions do not block writes");                                       \
        }                                                                                                              \
    } while (false)
#endif

namespace vsmm::test {
using namespace Qt::StringLiterals;

// returns false instead of QVERIFY, which would return from the helper and swallow the failure
[[nodiscard]] inline bool writeFile(const QString &path, const QByteArray &contents) {
    QFile file{path};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(contents) == contents.size();
}

[[nodiscard]] inline QByteArray readFile(const QString &path) {
    QFile file{path};
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}

// the host may have no trash for this filesystem, probe it with a scratch file rather than reading it
// back out of the outcome under test
[[nodiscard]] inline bool trashWorksIn(const QDir &dir) {
    const QString path = dir.absoluteFilePath(u"trash-probe.tmp"_s);
    if (!writeFile(path, "probe")) {
        return false;
    }
    if (QFile::moveToTrash(path)) {
        return true;
    }
    QFile::remove(path);
    return false;
}
} // namespace vsmm::test
