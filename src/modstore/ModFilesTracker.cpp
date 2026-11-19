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

#include "ModFilesTracker.hpp"

#include <IModStore.hpp>

#include <QFileInfo>
#include <QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(cModFilesTracker, "modstore");

namespace vsmm::modstore {
ModFilesTracker::ModFilesTracker(QObject *parent) : QObject{parent} {
    mDirChangeDelay.setSingleShot(true);
    mDirChangeDelay.setInterval(DIR_CHANGE_DELAY);
    connect(&mDirChangeDelay, &QTimer::timeout, this, &ModFilesTracker::onDirsChanged);
    connect(&mWatcher, &QFileSystemWatcher::directoryChanged, &mDirChangeDelay, qOverload<>(&QTimer::start));
    connect(&mWatcher, &QFileSystemWatcher::fileChanged, &mDirChangeDelay, qOverload<>(&QTimer::start));
}

void ModFilesTracker::reset(const QList<QDir> &modsDirs) {
    if (const QStringList watched = mWatcher.directories() + mWatcher.files(); !watched.isEmpty()) {
        mWatcher.removePaths(watched);
    }
    mKnownZips.clear();
    mDirChangeDelay.stop();
    for (const QDir &modsDir : modsDirs) {
        if (!modsDir.exists()) {
            continue;
        }
        QHash<QString, KnownZip> &knownZips = mKnownZips[modsDir.absolutePath()];
        QStringList watch{modsDir.absolutePath()};
        for (const QFileInfo &zip : modsDir.entryInfoList({IModStore::MOD_FILE_PATTERN}, IModStore::MOD_FILE_TYPES)) {
            knownZips.emplace(zip.absoluteFilePath(), zip.lastModified());
            watch.append(zip.absoluteFilePath());
        }
        mWatcher.addPaths(watch);
    }
}

void ModFilesTracker::refresh(const QString &path, const QString &modId) {
    const QFileInfo file{path};
    const auto knownZips = mKnownZips.find(file.absolutePath());
    if (knownZips == mKnownZips.end()) {
        return;
    }
    if (file.exists()) {
        knownZips->emplace(file.absoluteFilePath(), file.lastModified(), modId);
        mWatcher.addPath(file.absoluteFilePath());
    } else {
        knownZips->remove(file.absoluteFilePath());
    }
}

void ModFilesTracker::setModId(const QString &path, const QString &modId) {
    const QFileInfo file{path};
    if (const auto knownZips = mKnownZips.find(file.absolutePath()); knownZips != mKnownZips.end()) {
        if (const auto zip = knownZips->find(file.absoluteFilePath()); zip != knownZips->end()) {
            zip->mModId = modId;
        }
    }
}

QStringList ModFilesTracker::findDuplicates(const QString &modId, const QString &exceptPath) const {
    QStringList duplicates;
    for (const auto &knownZips : mKnownZips) {
        for (const auto &[path, zip] : knownZips.asKeyValueRange()) {
            if (zip.mModId == modId && path != exceptPath && QFileInfo::exists(path)) {
                duplicates.append(path);
            }
        }
    }
    return duplicates;
}

void ModFilesTracker::onDirsChanged() {
    QStringList found;
    QStringList removed;
    for (auto [dir, knownZips] : mKnownZips.asKeyValueRange()) {
        QHash<QString, KnownZip> current;
        for (const QFileInfo &zip : QDir{dir}.entryInfoList({IModStore::MOD_FILE_PATTERN}, IModStore::MOD_FILE_TYPES)) {
            const QString path = zip.absoluteFilePath();
            const auto previous = knownZips.find(path);
            if (previous != knownZips.end() && previous->mModified == zip.lastModified()) {
                current.insert(path, std::move(*previous));
                continue;
            }
            qCInfo(cModFilesTracker, "Mod file %s was %s", qUtf8Printable(path),
                   previous == knownZips.end() ? "added" : "changed");
            current.emplace(path, zip.lastModified());
            found.append(path);
        }
        for (const QString &path : knownZips.keys()) {
            if (!current.contains(path)) {
                qCInfo(cModFilesTracker, "Mod file %s was removed", qUtf8Printable(path));
                removed.append(path);
            }
        }

        if (!current.isEmpty()) {
            mWatcher.addPaths(current.keys());
        }
        knownZips = std::move(current);
    }

    if (!found.isEmpty() || !removed.isEmpty()) {
        emit filesChanged(found, removed);
    }
}
} // namespace vsmm::modstore
