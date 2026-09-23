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

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

// fake game exes are POSIX shell scripts, CreateProcess cannot run them
#ifdef Q_OS_WIN
#ifdef _MSC_VER
#pragma warning(disable : 4702)
#endif
#define SKIP_WITHOUT_POSIX_SHELL() QSKIP("Uses a POSIX shell script as a stand-in for the game executable")
#else
#define SKIP_WITHOUT_POSIX_SHELL() ((void)0)
#endif

namespace vsmm::test {
using namespace Qt::StringLiterals;

constexpr QLatin1StringView SUPPORTED_SETTINGS_VERSION{"1.16"};
constexpr QLatin1StringView CLIENT_SETTINGS_FILE{"clientsettings.json"};

[[nodiscard]] inline QJsonObject clientSettings(const QJsonArray &modPaths,
                                                QLatin1StringView settingsVersion = SUPPORTED_SETTINGS_VERSION) {
    return {{"stringSettings"_L1, QJsonObject{{"settingsVersion"_L1, settingsVersion}}},
            {"stringListSettings"_L1, QJsonObject{{"modPaths"_L1, modPaths}}}};
}

[[nodiscard]] inline QStringList toPaths(const QList<QDir> &dirs) {
    QStringList paths;
    paths.reserve(dirs.size());
    for (const auto &dir : dirs) {
        paths.append(dir.path());
    }
    return paths;
}

// returns false instead of QVERIFY, which would return from the helper and swallow the failure
[[nodiscard]] inline bool writeFile(const QString &path, const QByteArray &contents) {
    QFile file{path};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }
    return file.write(contents) == contents.size();
}

[[nodiscard]] inline bool writeClientSettings(const QDir &gameDir, const QByteArray &contents) {
    return writeFile(gameDir.absoluteFilePath(CLIENT_SETTINGS_FILE), contents);
}

[[nodiscard]] inline bool writeClientSettings(const QDir &gameDir, const QJsonObject &json) {
    return writeClientSettings(gameDir, QJsonDocument{json}.toJson());
}

[[nodiscard]] inline bool makeExecutable(const QString &path) {
    return QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
}

// stand-in for the game binary, prints a version for --version, otherwise touches marker
[[nodiscard]] inline QString writeFakeGameExe(const QDir &dir, const QString &version, const QString &marker = {}) {
    const QString path = dir.absoluteFilePath("fake-vintagestory.sh"_L1);
    const QByteArray script = "#!/bin/sh\n"
                              "if [ \"$1\" = \"--version\" ]; then\n"
                              "  echo " +
                              version.toUtf8() +
                              "\n"
                              "else\n"
                              "  touch \"" +
                              marker.toUtf8() +
                              "\"\n"
                              "fi\n";
    if (!writeFile(path, script) || !makeExecutable(path)) {
        return {};
    }
    return path;
}

// starts fine, then dies on a signal
[[nodiscard]] inline QString writeCrashingExe(const QDir &dir) {
    const QString path = dir.absoluteFilePath("crashing.sh"_L1);
    if (!writeFile(path, QByteArray{"#!/bin/sh\nkill -SEGV $$\n"}) || !makeExecutable(path)) {
        return {};
    }
    return path;
}

// starts fine but never exits, stands in for a binary that ignores --version
[[nodiscard]] inline QString writeHangingExe(const QDir &dir) {
    const QString path = dir.absoluteFilePath("hanging.sh"_L1);
    if (!writeFile(path, QByteArray{"#!/bin/sh\nsleep 30\n"}) || !makeExecutable(path)) {
        return {};
    }
    return path;
}
} // namespace vsmm::test
