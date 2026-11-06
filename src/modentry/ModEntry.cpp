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
#include <algorithm>
#include <optional>
#include <utility>

Q_STATIC_LOGGING_CATEGORY(cModEntry, "modentry");

namespace {
using namespace Qt::StringLiterals;
QUrl GetModUrlByAlias(QAnyStringView alias) { return u"https://mods.vintagestory.at/%1"_s.arg(alias); }
QUrl GetModUrlByAssetId(qint64 assetId) { return u"https://mods.vintagestory.at/show/mod/%1"_s.arg(assetId); }
} // namespace

namespace vsmm {

std::optional<ModRelease> ModRelease::fromJson(const QJsonObject &release) {
    const QString modVersion = release["modversion"_L1].toString();
    auto parsedVersion = semver::try_parse(modVersion.toStdString());
    if (!parsedVersion) {
        qCWarning(cModEntry, "%s: Cannot parse version '%s'", qUtf8Printable(release["modidstr"_L1].toString()),
                  qUtf8Printable(modVersion));
        return std::nullopt;
    }

    ModRelease result;
    result.mModVersion = std::move(parsedVersion).value();

    result.mReleaseId = release["releaseid"_L1].toInteger();
    result.mFileId = release["fileid"_L1].toInteger();
    result.mDownloads = release["downloads"_L1].toInteger();

    result.mModIdStr = release["modidstr"_L1].toString();
    result.mFileName = release["filename"_L1].toString();
    result.mChangelog = release["changelog"_L1].toString();

    // append 'Z' to the end of the string to treat it as UTC
    result.mCreated = QDateTime::fromString(release["created"_L1].toString() + u'Z', Qt::ISODate);

    result.mMainFile = QUrl{release["mainfile"_L1].toString(), QUrl::ParsingMode::StrictMode};

    for (const auto &tag : release["tags"_L1].toArray()) {
        const QString gameVersion = tag.toString();
        if (auto gameVersionParsed = semver::try_parse(gameVersion.toStdString()); gameVersionParsed) {
            result.mGameVersions.append(std::move(gameVersionParsed).value());
        } else {
            qCWarning(cModEntry, "%s: Cannot parse game version '%s'", qUtf8Printable(result.mModIdStr),
                      qUtf8Printable(gameVersion));
        }
    }

    return result;
}

ModScreenshot ModScreenshot::fromJson(const QJsonObject &screenshot) {
    ModScreenshot result;
    result.mFileId = screenshot["fileid"_L1].toInteger();

    result.mMainFile = QUrl{screenshot["mainfile"_L1].toString(), QUrl::ParsingMode::StrictMode};
    result.mThumbnail = QUrl{screenshot["thumbnailfilename"_L1].toString(), QUrl::ParsingMode::StrictMode};

    result.mFileName = screenshot["filename"_L1].toString();

    result.mCreated = QDateTime::fromString(screenshot["created"_L1].toString() + u'Z', Qt::ISODate);
    return result;
}

ModOnlineInfo ModOnlineInfo::fromJson(const QJsonObject &mod) {
    ModOnlineInfo info;
    info.mModId = mod["modid"_L1].toInteger();
    info.mAssetId = mod["assetid"_L1].toInteger();
    info.mDownloads = mod["downloads"_L1].toInteger();
    info.mFollows = mod["follows"_L1].toInteger();
    info.mComments = mod["comments"_L1].toInteger();
    info.mTrendingPoints = mod["trendingpoints"_L1].toInteger();

    info.mName = mod["name"_L1].toString().trimmed();
    info.mAuthor = mod["author"_L1].toString().trimmed();
    info.mUrlAlias = mod["urlalias"_L1].toString().trimmed();
    info.mSide = mod["side"_L1].toString();
    info.mType = mod["type"_L1].toString();
    info.mText = mod["text"_L1].toString().trimmed();

    // append 'Z' to the end of the string to treat it as UTC
    info.mCreated = QDateTime::fromString(mod["created"_L1].toString() + u'Z', Qt::ISODate);
    info.mLastModified = QDateTime::fromString(mod["lastmodified"_L1].toString() + u'Z', Qt::ISODate);
    info.mLastReleased = QDateTime::fromString(mod["lastreleased"_L1].toString() + u'Z', Qt::ISODate);

    info.mLogo = QUrl{mod["logofile"_L1].toString(), QUrl::ParsingMode::StrictMode};
    info.mHomepage = QUrl{mod["homepageurl"_L1].toString(), QUrl::ParsingMode::StrictMode};
    info.mSourceCode = QUrl{mod["sourcecodeurl"_L1].toString(), QUrl::ParsingMode::StrictMode};
    info.mIssueTracker = QUrl{mod["issuetrackerurl"_L1].toString(), QUrl::ParsingMode::StrictMode};
    info.mWiki = QUrl{mod["wikiurl"_L1].toString(), QUrl::ParsingMode::StrictMode};
    info.mTrailerVideo = QUrl{mod["trailervideourl"_L1].toString(), QUrl::ParsingMode::StrictMode};

    for (const auto &tag : mod["tags"_L1].toArray()) {
        if (!tag.isString()) {
            continue;
        }
        info.mTags.append(tag.toString());
    }

    std::ranges::for_each(mod["releases"_L1].toArray(), [&info](const auto &release) {
        if (auto releaseResult = ModRelease::fromJson(release.toObject()); releaseResult) {
            info.mReleases.append(std::move(*releaseResult));
        }
    });

    std::ranges::for_each(mod["screenshots"_L1].toArray(), [&info](const auto &screenshot) {
        info.mScreenshots.append(ModScreenshot::fromJson(screenshot.toObject()));
    });

    return info;
}

ModEntry::ModEntry(ModLocalInfo info)
    : mName{std::move(info.mName)}, mVersion{std::move(info.mVersion)}, mAuthors{std::move(info.mAuthors)},
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

QStringList ModEntry::getAuthors() const {
    if (mOnlineInfo.mAuthor.isEmpty()) {
        return mAuthors;
    }
    return {mOnlineInfo.mAuthor};
}

const semver::version<> &ModEntry::getVersion() const { return mVersion; }
const QFileInfo &ModEntry::getFileInfo() const { return mFileInfo; }
bool ModEntry::isMarkedForUpdate() const { return mMarkedForUpdate; }
bool ModEntry::isFavorite() const { return mFavorite; }
bool ModEntry::hasUpdate() const { return mLatestVersion.has_value(); }
void ModEntry::setMarkedForUpdate(bool marked) { mMarkedForUpdate = marked; }
void ModEntry::setFavorite(bool favorite) { mFavorite = favorite; }

void ModEntry::initOnlineInfo(ModOnlineInfo onlineInfo, const semver::version<> &gameVersion,
                              bool cfgIncludePrerelease) {
    mOnlineInfo = std::move(onlineInfo);
    mUrl = !mOnlineInfo.mUrlAlias.isEmpty() ? GetModUrlByAlias(mOnlineInfo.mUrlAlias)
                                            : GetModUrlByAssetId(mOnlineInfo.mAssetId);

    mLatestVersion = findLatestRelease(gameVersion, cfgIncludePrerelease);

    qCDebug(cModEntry, "Retrieved mod info for %s", qUtf8Printable(mOnlineInfo.mName));
}

const QUrl &ModEntry::getUrl() const { return mUrl; }
const ModRelease *ModEntry::getLatestVersion() const { return mLatestVersion ? &*mLatestVersion : nullptr; }
const QStringList &ModEntry::getTags() const { return mOnlineInfo.mTags; }
QStringView ModEntry::getSide() const { return mOnlineInfo.mSide; }
const QUrl &ModEntry::getLogoUrl() const { return mOnlineInfo.mLogo; }

std::optional<ModRelease> ModEntry::findLatestRelease(const semver::version<> &gameVersion,
                                                      bool cfgIncludePrerelease) const {
    const auto supportsGameVersion = [&gameVersion](const QList<semver::version<>> &supportedVersions) {
        return std::ranges::any_of(supportedVersions, [&gameVersion](const auto &supportedVersion) {
            // treat patch numbers as compatible
            return supportedVersion.major() == gameVersion.major() && supportedVersion.minor() == gameVersion.minor();
        });
    };

    const bool includePrerelease = !mVersion.prerelease_tag().empty() || cfgIncludePrerelease;
    std::optional<ModRelease> latest;
    for (const auto &release : mOnlineInfo.mReleases) {
        if (release.mModVersion <= mVersion) {
            continue;
        }

        // only include prerelease if mod is already a prerelease, or it is set in app cfg explicitly
        if (!release.mModVersion.prerelease_tag().empty() && !includePrerelease) {
            continue;
        }

        if (!supportsGameVersion(release.mGameVersions) || (latest && release.mModVersion <= latest->mModVersion)) {
            continue;
        }

        latest = release;
    }
    return latest;
}

} // namespace vsmm
