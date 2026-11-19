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

#include <ModEntryExport.hpp>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonObject>
#include <semver.hpp>

namespace vsmm {

struct MODENTRY_EXPORT ModLocalInfo {
    Q_GADGET
    Q_PROPERTY(QString name MEMBER mName)
    Q_PROPERTY(QString id MEMBER mId)
    Q_PROPERTY(QString description MEMBER mDescription)
    Q_PROPERTY(QString side MEMBER mSide)
    Q_PROPERTY(QString version READ versionString)
    Q_PROPERTY(QStringList authors MEMBER mAuthors)
    Q_PROPERTY(QStringList dependencies MEMBER mDependencies)

    Q_PROPERTY(QString error MEMBER mError)

  public:
    QString mName{}, mId{}, mDescription{QStringLiteral(u"None")}, mSide{QStringLiteral(u"Unknown")}, mError{};
    QStringList mAuthors{{QStringLiteral(u"Unknown")}}, mDependencies{};
    semver::version<> mVersion{};
    QFileInfo mFileInfo{};
    [[nodiscard]] QString toString() const {
        return QStringLiteral(u"%1@%2").arg(mId).arg(QString::fromStdString(mVersion.to_string()));
    }
    [[nodiscard]] QString versionString() const { return QString::fromStdString(mVersion.to_string()); }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator QString() const { return toString(); }
};

struct MODENTRY_EXPORT ModRelease {
    qint64 mReleaseId{}, mFileId{}, mDownloads{};
    QString mModIdStr, mFileName, mChangelog;
    semver::version<> mModVersion;
    QDateTime mCreated;
    QUrl mMainFile;
    QList<semver::version<>> mGameVersions; // release "tags"
    bool operator==(const ModRelease &) const = default;

    [[nodiscard]] static std::optional<ModRelease> fromJson(const QJsonObject &release);
};

struct MODENTRY_EXPORT ModScreenshot {
    qint64 mFileId{};
    QUrl mMainFile, mThumbnail;
    QString mFileName;
    QDateTime mCreated;
    bool operator==(const ModScreenshot &) const = default;

    [[nodiscard]] static ModScreenshot fromJson(const QJsonObject &screenshot);
};

struct MODENTRY_EXPORT ModOnlineInfo {
    qint64 mModId{}, mAssetId{}, mDownloads{}, mFollows{}, mComments{}, mTrendingPoints{};
    QString mName, mAuthor, mUrlAlias, mSide, mType, mText;
    QDateTime mCreated, mLastModified, mLastReleased; // server time, parsed as UTC
    QUrl mLogo, mHomepage, mSourceCode, mIssueTracker, mWiki, mTrailerVideo;
    QStringList mTags;
    QList<ModRelease> mReleases;
    QList<ModScreenshot> mScreenshots;
    bool operator==(const ModOnlineInfo &) const = default;

    [[nodiscard]] static ModOnlineInfo fromJson(const QJsonObject &mod);
};

class MODENTRY_EXPORT ModEntry {
  public:
    explicit ModEntry(ModLocalInfo info);

    [[nodiscard]] QString toString() const;

    // ReSharper disable once CppNonExplicitConversionOperator
    operator QString() const { return toString(); }

    [[nodiscard]] QStringView getId() const;
    [[nodiscard]] QStringView getName() const;
    [[nodiscard]] QStringList getAuthors() const;
    [[nodiscard]] const semver::version<> &getVersion() const;
    [[nodiscard]] const QFileInfo &getFileInfo() const;
    [[nodiscard]] bool isMarkedForUpdate() const;
    [[nodiscard]] bool isFavorite() const;
    [[nodiscard]] bool hasUpdate() const;
    void setMarkedForUpdate(bool marked);
    void setFavorite(bool favorite);

    void initOnlineInfo(ModOnlineInfo onlineInfo, const semver::version<> &gameVersion, bool cfgIncludePrerelease);

    // Online
    [[nodiscard]] const QUrl &getUrl() const;
    [[nodiscard]] const ModRelease *getLatestVersion() const;
    [[nodiscard]] const QStringList &getTags() const;
    [[nodiscard]] QStringView getSide() const;
    [[nodiscard]] const QUrl &getLogoUrl() const;

  private:
    [[nodiscard]] std::optional<ModRelease> findLatestRelease(const semver::version<> &gameVersion,
                                                              bool cfgIncludePrerelease) const;

    QString mName;
    semver::version<> mVersion;
    QStringList mAuthors;
    QString mModId;
    QFileInfo mFileInfo;
    ModOnlineInfo mOnlineInfo;
    QUrl mUrl;
    bool mMarkedForUpdate{false};
    bool mFavorite{false};
    std::optional<ModRelease> mLatestVersion;
};
} // namespace vsmm
