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

#include <ModEntry.hpp>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

#include <semver.hpp>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

// a read-only dir does not stop a write on Windows, and root ignores the mode bits everywhere
#ifdef Q_OS_WIN
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

[[nodiscard]] inline semver::version<> ver(const QString &version) {
    semver::version<> parsed;
    const auto result = semver::parse(version.toStdString(), parsed);
    // not Q_ASSERT, that would abort the run and behave differently in release builds
    QTest::qVerify(static_cast<bool>(result), "semver::parse(version)", qPrintable(version), __FILE__, __LINE__);
    return parsed;
}

[[nodiscard]] inline QString str(const semver::version<> &version) {
    return QString::fromStdString(version.to_string());
}

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

// the store only copies, stats and deletes mod zips, it never opens one, so a stub file is enough
[[nodiscard]] inline QFileInfo writeStubZip(const QDir &dir, const QString &fileName) {
    const QString path = dir.absoluteFilePath(fileName);
    return writeFile(path, "not a real zip") ? QFileInfo{path} : QFileInfo{};
}

[[nodiscard]] inline ModEntry::LocalInfo localInfo(const QString &id, const QString &version, const QFileInfo &file) {
    return {.mName = u"%1 (local)"_s.arg(id),
            .mId = id,
            .mAuthor = u"local author"_s,
            .mVersion = ver(version),
            .mFileInfo = file};
}

// one entry of the API releases array, supported game versions go into tags
[[nodiscard]] inline QJsonObject release(const QString &modVersion, const QStringList &gameVersions) {
    QJsonArray tags;
    for (const auto &gameVersion : gameVersions) {
        tags.append(gameVersion);
    }
    return {{"modversion"_L1, modVersion},
            {"tags"_L1, tags},
            {"filename"_L1, u"CarryOn_v%1.zip"_s.arg(modVersion)},
            {"mainfile"_L1, u"https://mods.vintagestory.at/download?fileid=%1"_s.arg(modVersion)}};
}

// the mod object of an API response, trimmed down to what ModEntry reads
[[nodiscard]] inline QJsonObject modJson(const QJsonArray &releases) {
    return {{"name"_L1, "Carry On"_L1}, {"author"_L1, "NerdScurvy"_L1},
            {"type"_L1, "mod"_L1},      {"urlalias"_L1, "carryon"_L1},
            {"assetid"_L1, 4405},       {"tags"_L1, QJsonArray{"Storage"_L1, "QoL"_L1}},
            {"releases"_L1, releases}};
}
} // namespace vsmm::test
