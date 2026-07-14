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

#include "ModEntry.hpp"
#include <QJsonArray>
#include <optional>
#include <utility>

#include <semver/semver.hpp>

namespace {
using namespace Qt::StringLiterals;
QUrl GetModUrlByAlias(QAnyStringView alias) { return u"https://mods.vintagestory.at/%1"_s.arg(alias); }
QUrl GetModUrlByAssetId(qint64 assetId) { return u"https://mods.vintagestory.at/show/mod/%1"_s.arg(assetId); }
} // namespace

namespace vsmm {

ModEntry::ModEntry(LocalInfo info)
    : mName{std::move(info.mName)}, mVersion{std::move(info.mVersion)}, mAuthor{std::move(info.mAuthor)},
      mModId{std::move(info.mId)} {
    mFileInfo = std::move(info.mFileInfo);
}

QString ModEntry::toString() const { return u"%1@%2"_s.arg(mModId).arg(QString::fromStdString(mVersion.str())); }

QStringView ModEntry::getId() const { return mModId; }

QStringView ModEntry::getName() const {
    if (mOnlineInfo.mName.isEmpty()) {
        return mName;
    }
    return mOnlineInfo.mName;
}

QStringView ModEntry::getAuthor() const {
    if (mOnlineInfo.mAuthor.isEmpty()) {
        return mAuthor;
    }
    return mOnlineInfo.mAuthor;
}

const semver::version &ModEntry::getVersion() const { return mVersion; }
const QFileInfo &ModEntry::getFileInfo() const { return mFileInfo; }
bool ModEntry::isMarkedForUpdate() const { return mMarkedForUpdate; }
bool ModEntry::isFavorite() const { return mFavorite; }
void ModEntry::setMarkedForUpdate(bool marked) { mMarkedForUpdate = marked; }
void ModEntry::setFavorite(bool favorite) { mFavorite = favorite; }

void ModEntry::initOnlineInfo(QJsonObject json, const semver::version &gameVersion, bool cfgIncludePrerelease) {
    initName(json);
    initModUrl(json);
    initAuthor(json);
    initTags(json);
    initLatestRelease(json, gameVersion, cfgIncludePrerelease);
    initType(json);

    qDebug() << u"Retrieved mod info for %1"_s.arg(mOnlineInfo.mName);
}

const QUrl &ModEntry::getUrl() const { return mOnlineInfo.mUrl; }
const ModEntry::LatestVersion &ModEntry::getLatestVersion() const { return mOnlineInfo.mLatestVersion; }
const QStringList &ModEntry::getTags() const { return mOnlineInfo.mTags; }
QStringView ModEntry::getType() const { return mOnlineInfo.mType; }
bool ModEntry::hasUpdate() const { return mOnlineInfo.mLatestVersion.mHasUpdate; }

void ModEntry::initName(const QJsonObject &json) {
    if (!json["name"].isString()) {
        qWarning() << u"%1: Invalid JSON format: name is not a string"_s.arg(mName);
        return;
    }
    mOnlineInfo.mName = json["name"].toString().trimmed();
}

void ModEntry::initLatestRelease(const QJsonObject &json, const semver::version &gameVersion,
                                 bool cfgIncludePrerelease) {
    if (!json["releases"].isArray()) {
        qWarning() << u"%1: Invalid JSON format: releases is not an array"_s.arg(mOnlineInfo.mName);
        return;
    }

    auto jsonReleaseArr = json["releases"].toArray();
    if (jsonReleaseArr.isEmpty()) {
        qWarning() << u"%1: Invalid JSON format: releases array is empty"_s.arg(mOnlineInfo.mName);
        return;
    }

    auto latestRelease = getLatestVersion(std::move(jsonReleaseArr), gameVersion, cfgIncludePrerelease);
    if (latestRelease.first.empty()) {
        return;
    }

    mOnlineInfo.mLatestVersion.mVersion = std::move(latestRelease.second);
    mOnlineInfo.mLatestVersion.mFileName = latestRelease.first["filename"_L1].toString();
    mOnlineInfo.mLatestVersion.mUrl = latestRelease.first["mainfile"_L1].toString();
    mOnlineInfo.mLatestVersion.mHasUpdate = true;
}

void ModEntry::initAuthor(const QJsonObject &json) {
    if (!json["author"].isString()) {
        qWarning() << u"%1: Invalid JSON format: author is not a string"_s.arg(mOnlineInfo.mName);
        return;
    }

    mAuthor = json["author"].toString();
}

void ModEntry::initTags(const QJsonObject &json) {
    if (!json["tags"].isArray()) {
        qWarning() << u"%1: Invalid JSON format: tags is not a list"_s.arg(mOnlineInfo.mName);
        return;
    }

    for (const auto &tag : json["tags"].toArray()) {
        if (tag.isNull()) {
            continue;
        }
        mOnlineInfo.mTags.append(tag.toString());
    }
}

void ModEntry::initModUrl(const QJsonObject &json) {
    if (!json["urlalias"].isNull() && json["urlalias"].isString()) {
        mOnlineInfo.mUrl = GetModUrlByAlias(json["urlalias"].toString());
        return;
    }

    if (!json["assetid"].isDouble()) {
        qWarning() << u"%1: Invalid JSON format: assetid is not a number"_s.arg(mOnlineInfo.mName);
        return;
    }

    mOnlineInfo.mUrl = GetModUrlByAssetId(json["assetid"].toInteger());
}

void ModEntry::initType(const QJsonObject &json) {
    if (!json["type"].isString()) {
        qWarning() << u"%1: Invalid JSON format: type is not a string"_s.arg(mOnlineInfo.mName);
        return;
    }

    mOnlineInfo.mType = json["type"].toString();
}

QPair<QJsonObject, semver::version> ModEntry::getLatestVersion(QJsonArray releases, const semver::version &gameVersion,
                                                               bool cfgIncludePrerelease) const {
    const auto tryParse = [this](const QString &version) -> std::optional<semver::version> {
        try {
            return semver::version::parse(version.toStdString());
        } catch (const semver::semver_exception &e) {
            qCritical() << u"%1: Cannot parse version '%2': %3"_s.arg(mOnlineInfo.mName, version).arg(e.what());
            return std::nullopt;
        }
    };

    const bool includePrerelease = mVersion.is_prerelease() || cfgIncludePrerelease;
    for (const auto &release : releases) {
        if (!release["modversion"_L1].isString()) {
            qCritical() << u"%1: Invalid JSON format: no modversion"_s.arg(mOnlineInfo.mName);
            continue;
        }

        const auto releaseVersion = tryParse(release["modversion"_L1].toString());

        if (!releaseVersion) {
            continue;
        }

        // releases are sorted from latest -> oldest
        if (*releaseVersion <= mVersion) {
            break;
        }

        // only include prerelease if mod is already a prerelease, or it is set in app cfg explicitly
        if (releaseVersion->is_prerelease() && !includePrerelease) {
            continue;
        }
        if (!release["tags"_L1].isArray()) {
            qCritical() << u"%1: Invalid JSON format: release tags is not an array"_s.arg(mOnlineInfo.mName);
            continue;
        }

        for (const auto &tag : release["tags"_L1].toArray()) {
            const auto supported = tryParse(tag.toString());
            if (!supported) {
                continue;
            }

            // treat patch numbers as compatible
            if (supported->major() == gameVersion.major() && supported->minor() == gameVersion.minor()) {
                return {release.toObject(), *releaseVersion};
            }
        }
    }
    return {{}, semver::version{}};
}

} // namespace vsmm
