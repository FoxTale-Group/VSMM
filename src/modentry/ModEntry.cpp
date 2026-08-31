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
#include <QLoggingCategory>
#include <optional>
#include <utility>

Q_STATIC_LOGGING_CATEGORY(cModEntry, "modentry");

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

QString ModEntry::toString() const { return u"%1@%2"_s.arg(mModId).arg(QString::fromStdString(mVersion.to_string())); }

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

const semver::version<> &ModEntry::getVersion() const { return mVersion; }
const QFileInfo &ModEntry::getFileInfo() const { return mFileInfo; }
bool ModEntry::isMarkedForUpdate() const { return mMarkedForUpdate; }
bool ModEntry::isFavorite() const { return mFavorite; }
void ModEntry::setMarkedForUpdate(bool marked) { mMarkedForUpdate = marked; }
void ModEntry::setFavorite(bool favorite) { mFavorite = favorite; }

void ModEntry::initOnlineInfo(QJsonObject json, const semver::version<> &gameVersion, bool cfgIncludePrerelease) {
    initName(json);
    initModUrl(json);
    initAuthor(json);
    initTags(json);
    initLatestRelease(json, gameVersion, cfgIncludePrerelease);
    initType(json);
    initLogoUrl(json);

    qCDebug(cModEntry, "Retrieved mod info for %s", qUtf8Printable(mOnlineInfo.mName));
}

const QUrl &ModEntry::getUrl() const { return mOnlineInfo.mUrl; }
const ModEntry::LatestVersion &ModEntry::getLatestVersion() const { return mOnlineInfo.mLatestVersion; }
const QStringList &ModEntry::getTags() const { return mOnlineInfo.mTags; }
QStringView ModEntry::getType() const { return mOnlineInfo.mType; }
bool ModEntry::hasUpdate() const { return mOnlineInfo.mLatestVersion.mHasUpdate; }
const QUrl &ModEntry::getLogoUrl() const { return mOnlineInfo.mLogoUrl; }

void ModEntry::initName(const QJsonObject &json) {
    if (!json["name"_L1].isString()) {
        qCWarning(cModEntry, "%s: Invalid JSON format: name is not a string", qUtf8Printable(mName));
        return;
    }
    mOnlineInfo.mName = json["name"].toString().trimmed();
}

void ModEntry::initLatestRelease(const QJsonObject &json, const semver::version<> &gameVersion,
                                 bool cfgIncludePrerelease) {
    if (!json["releases"_L1].isArray()) {
        qCWarning(cModEntry, "%s: Invalid JSON format: releases is not an array", qUtf8Printable(mOnlineInfo.mName));
        return;
    }

    auto jsonReleaseArr = json["releases"_L1].toArray();
    if (jsonReleaseArr.isEmpty()) {
        qCWarning(cModEntry, "%s: Invalid JSON format: releases array is empty", qUtf8Printable(mOnlineInfo.mName));
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
    if (!json["author"_L1].isString()) {
        qCWarning(cModEntry, "%s: Invalid JSON format: author is not a string", qUtf8Printable(mOnlineInfo.mName));
        return;
    }

    mOnlineInfo.mAuthor = json["author"_L1].toString();
}

void ModEntry::initTags(const QJsonObject &json) {
    if (!json["tags"_L1].isArray()) {
        qCWarning(cModEntry, "%s: Invalid JSON format: tags is not a list", qUtf8Printable(mOnlineInfo.mName));
        return;
    }

    for (const auto &tag : json["tags"_L1].toArray()) {
        if (tag.isNull()) {
            continue;
        }
        mOnlineInfo.mTags.append(tag.toString());
    }
}

void ModEntry::initModUrl(const QJsonObject &json) {
    if (!json["urlalias"_L1].isNull() && json["urlalias"_L1].isString()) {
        mOnlineInfo.mUrl = GetModUrlByAlias(json["urlalias"_L1].toString());
        return;
    }

    if (!json["assetid"_L1].isDouble()) {
        qCWarning(cModEntry, "%s: Invalid JSON format: assetid is not a number", qUtf8Printable(mOnlineInfo.mName));
        return;
    }

    mOnlineInfo.mUrl = GetModUrlByAssetId(json["assetid"_L1].toInteger());
}

void ModEntry::initType(const QJsonObject &json) {
    if (!json["type"_L1].isString()) {
        qCWarning(cModEntry, "%s: Invalid JSON format: type is not a string", qUtf8Printable(mOnlineInfo.mName));
        return;
    }

    mOnlineInfo.mType = json["type"_L1].toString();
}

void ModEntry::initLogoUrl(const QJsonObject &json) {
    if (!json[ONLINE_LOGOFILE_JSON_KEY].isString()) {
        qCDebug(cModEntry, "%s: no %s in the online info", qUtf8Printable(mOnlineInfo.mName),
                qUtf8Printable(ONLINE_LOGOFILE_JSON_KEY));
        return;
    }

    mOnlineInfo.mLogoUrl = json[ONLINE_LOGOFILE_JSON_KEY].toString();
}

QPair<QJsonObject, semver::version<>>
ModEntry::getLatestVersion(QJsonArray releases, const semver::version<> &gameVersion, bool cfgIncludePrerelease) const {
    const auto tryParse = [this](const QString &version) -> std::optional<semver::version<>> {
        semver::version<> returnVersion;
        if (const auto result = semver::parse(version.toStdString(), returnVersion); !result) {
            qCCritical(cModEntry, "%s: Cannot parse version '%s'", qUtf8Printable(mOnlineInfo.mName),
                       qUtf8Printable(version));
            return std::nullopt;
        }
        return returnVersion;
    };
    const auto supportsGameVersion = [&gameVersion, &tryParse](const QJsonArray &tags) {
        for (const auto &tag : tags) {
            const auto supported = tryParse(tag.toString());
            if (!supported) {
                continue;
            }

            // treat patch numbers as compatible
            if (supported->major() == gameVersion.major() && supported->minor() == gameVersion.minor()) {
                return true;
            }
        }
        return false;
    };

    const bool includePrerelease = !mVersion.prerelease_tag().empty() || cfgIncludePrerelease;
    QPair<QJsonObject, semver::version<>> latest;
    for (const auto &release : releases) {
        if (!release["modversion"_L1].isString()) {
            qCCritical(cModEntry, "%s: Invalid JSON format: no modversion", qUtf8Printable(mOnlineInfo.mName));
            continue;
        }

        const auto releaseVersion = tryParse(release["modversion"_L1].toString());

        // the API orders releases by release date
        if (!releaseVersion || *releaseVersion <= mVersion ||
            // only include prerelease if mod is already a prerelease, or it is set in app cfg explicitly
            (!releaseVersion->prerelease_tag().empty() && !includePrerelease)) {
            continue;
        }

        if (!release["tags"_L1].isArray()) {
            qCCritical(cModEntry, "%s: Invalid JSON format: release tags is not an array",
                       qUtf8Printable(mOnlineInfo.mName));
            continue;
        }

        if (!supportsGameVersion(release["tags"_L1].toArray()) ||
            (!latest.first.isEmpty() && *releaseVersion <= latest.second)) {
            continue;
        }

        latest = {release.toObject(), *releaseVersion};
    }
    return latest;
}

} // namespace vsmm
