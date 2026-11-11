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

#include "ModInfoCache.hpp"
#include "ModInfoCacheSql.hpp"
#include "ModInfoCacheValues.hpp"
#include "ModListWriter.hpp"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QLoggingCategory>
#include <QScopeGuard>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>
#include <QVariant>

#include <optional>

Q_STATIC_LOGGING_CATEGORY(cModInfoCache, "modinfocache");

using namespace Qt::StringLiterals;
using namespace vsmm::modinfocache;

namespace vsmm {
ModInfoCache::ModInfoCache(QObject *parent) : IModInfoCache{parent} {
    // SQLite allows only one writer at a time
    mWriter.setMaxThreadCount(1);

    mDatabase = QSqlDatabase::addDatabase(u"QSQLITE"_s, CONNECTION_NAME);
    mDatabase.setDatabaseName(mDbLocation);

    if (!QDir{}.mkpath(QFileInfo{mDbLocation}.absolutePath())) {
        qCWarning(cModInfoCache, "Failed to create directory for database %s", qUtf8Printable(mDbLocation));
        return;
    }

    if (!mDatabase.open()) {
        qCWarning(cModInfoCache, "Failed to open database %s: %s", qUtf8Printable(mDbLocation),
                  qUtf8Printable(mDatabase.lastError().text()));
        return;
    }

    QSqlQuery query{mDatabase};
    if (!query.exec(u"PRAGMA journal_mode = WAL"_s) || !query.next() || query.value(0).toString() != "wal"_L1) {
        qCWarning(cModInfoCache, "Cannot switch %s to WAL", qUtf8Printable(mDbLocation));
        return;
    }

    if (!query.exec(u"PRAGMA user_version"_s) || !query.next()) {
        qCWarning(cModInfoCache, "Cannot read the schema version: %s", qUtf8Printable(query.lastError().text()));
        return;
    }

    // rebuild the cache if version is different
    if (query.value(0).toInt() != CACHE_VERSION) {
        if (!resetSchema(query)) {
            return;
        }
    }

    if (!query.exec(u"PRAGMA foreign_keys = ON"_s)) {
        qCWarning(cModInfoCache, "Cannot enable foreign keys: %s", qUtf8Printable(query.lastError().text()));
        return;
    }

    query.prepare(u"DELETE FROM mod_details WHERE fetched_at < :oldest"_s);
    query.bindValue(u":oldest"_s, QDateTime::currentSecsSinceEpoch() - PRUNE_AFTER.count());
    if (!query.exec()) {
        qCWarning(cModInfoCache, "Cannot prune old mod details: %s", qUtf8Printable(query.lastError().text()));
        return;
    }

    if (!createStatements()) {
        return;
    }

    qCInfo(cModInfoCache, "Database opened successfully");
}

ModInfoCache::~ModInfoCache() {
    mWriter.waitForDone();
    mStatements = {};
    mDatabase.close();
    mDatabase = {};
    QSqlDatabase::removeDatabase(CONNECTION_NAME);
}

std::optional<ModOnlineInfo> ModInfoCache::getModInfo(const QString &modId) const {
    if (!getQuery(QueryType::Details)) {
        return std::nullopt;
    }

    QSqlQuery &details = *getQuery(QueryType::Details);
    QSqlQuery &tags = *getQuery(QueryType::Tags);
    QSqlQuery &screenshots = *getQuery(QueryType::Screenshots);
    QSqlQuery &releases = *getQuery(QueryType::Releases);
    QSqlQuery &releaseVersions = *getQuery(QueryType::ReleaseVersions);

    const auto finishAllQueries = qScopeGuard([&] {
        details.finish();
        tags.finish();
        screenshots.finish();
        releases.finish();
        releaseVersions.finish();
    });

    const auto execQuery = [&modId](QSqlQuery &query) {
        query.bindValue(u":mod_id"_s, modId);
        if (!query.exec()) {
            qCWarning(cModInfoCache, "Cannot read details of %s: %s", qUtf8Printable(modId),
                      qUtf8Printable(query.lastError().text()));
            return false;
        }
        return true;
    };

    // not stored or different lastreleased
    if (!execQuery(details) || !details.next()) {
        return std::nullopt;
    }

    ModOnlineInfo info;
    info.mModId = details.value("modid"_L1).toLongLong();
    info.mAssetId = details.value("assetid"_L1).toLongLong();
    info.mName = details.value("name"_L1).toString();
    info.mAuthor = details.value("author"_L1).toString();
    info.mUrlAlias = details.value("urlalias"_L1).toString();
    info.mSide = details.value("side"_L1).toString();
    info.mType = details.value("type"_L1).toString();
    info.mText = details.value("text"_L1).toString();
    info.mLogo = QUrl{details.value("logofile"_L1).toString()};
    info.mHomepage = QUrl{details.value("homepageurl"_L1).toString()};
    info.mSourceCode = QUrl{details.value("sourcecodeurl"_L1).toString()};
    info.mIssueTracker = QUrl{details.value("issuetrackerurl"_L1).toString()};
    info.mWiki = QUrl{details.value("wikiurl"_L1).toString()};
    info.mTrailerVideo = QUrl{details.value("trailervideourl"_L1).toString()};
    info.mDownloads = details.value("downloads"_L1).toLongLong();
    info.mFollows = details.value("follows"_L1).toLongLong();
    info.mComments = details.value("comments"_L1).toLongLong();
    info.mTrendingPoints = details.value("trendingpoints"_L1).toLongLong();
    info.mCreated = dateOrInvalid(details.value("created"_L1));
    info.mLastModified = dateOrInvalid(details.value("lastmodified"_L1));
    info.mLastReleased = dateOrInvalid(details.value("lastreleased"_L1));

    if (!execQuery(tags)) {
        return std::nullopt;
    }
    const int tagName = tags.record().indexOf("tag"_L1);
    while (tags.next()) {
        info.mTags.append(tags.value(tagName).toString());
    }

    if (!execQuery(screenshots)) {
        return std::nullopt;
    }

    const QSqlRecord screenshotColumns = screenshots.record();
    const int screenshotFileId = screenshotColumns.indexOf("fileid"_L1);
    const int screenshotMainFile = screenshotColumns.indexOf("mainfile"_L1);
    const int screenshotFileName = screenshotColumns.indexOf("filename"_L1);
    const int screenshotThumbnail = screenshotColumns.indexOf("thumbnailfilename"_L1);
    const int screenshotCreated = screenshotColumns.indexOf("created"_L1);
    while (screenshots.next()) {
        ModScreenshot screenshot;
        screenshot.mFileId = screenshots.value(screenshotFileId).toLongLong();
        screenshot.mMainFile = QUrl{screenshots.value(screenshotMainFile).toString()};
        screenshot.mFileName = screenshots.value(screenshotFileName).toString();
        screenshot.mThumbnail = QUrl{screenshots.value(screenshotThumbnail).toString()};
        screenshot.mCreated = dateOrInvalid(screenshots.value(screenshotCreated));
        info.mScreenshots.append(std::move(screenshot));
    }

    if (!execQuery(releases)) {
        return std::nullopt;
    }
    const QSqlRecord releaseColumns = releases.record();
    const int releasePosition = releaseColumns.indexOf("position"_L1);
    const int releaseId = releaseColumns.indexOf("releaseid"_L1);
    const int releaseModIdStr = releaseColumns.indexOf("modidstr"_L1);
    const int releaseModVersion = releaseColumns.indexOf("modversion"_L1);
    const int releaseFileName = releaseColumns.indexOf("filename"_L1);
    const int releaseMainFile = releaseColumns.indexOf("mainfile"_L1);
    const int releaseFileId = releaseColumns.indexOf("fileid"_L1);
    const int releaseDownloads = releaseColumns.indexOf("downloads"_L1);
    const int releaseCreated = releaseColumns.indexOf("created"_L1);
    const int releaseChangelog = releaseColumns.indexOf("changelog"_L1);

    // stored position to index in mReleases
    QHash<qint64, qsizetype> releaseIndex;
    while (releases.next()) {
        auto version = parseVersion(releases.value(releaseModVersion), modId);
        if (!version) {
            continue;
        }
        ModRelease release;
        release.mReleaseId = releases.value(releaseId).toLongLong();
        release.mModIdStr = releases.value(releaseModIdStr).toString();
        release.mModVersion = std::move(version).value();
        release.mFileName = releases.value(releaseFileName).toString();
        release.mMainFile = QUrl{releases.value(releaseMainFile).toString()};
        release.mFileId = releases.value(releaseFileId).toLongLong();
        release.mDownloads = releases.value(releaseDownloads).toLongLong();
        release.mCreated = dateOrInvalid(releases.value(releaseCreated));
        release.mChangelog = releases.value(releaseChangelog).toString();
        releaseIndex.insert(releases.value(releasePosition).toLongLong(), info.mReleases.size());
        info.mReleases.append(std::move(release));
    }

    if (!execQuery(releaseVersions)) {
        return std::nullopt;
    }
    const QSqlRecord versionColumns = releaseVersions.record();
    const int versionReleasePosition = versionColumns.indexOf("release_position"_L1);
    const int versionGameVersion = versionColumns.indexOf("gameversion"_L1);
    while (releaseVersions.next()) {
        const auto index = releaseIndex.constFind(releaseVersions.value(versionReleasePosition).toLongLong());
        // check if version of release was correctly parsed above
        if (index == releaseIndex.constEnd()) {
            continue;
        }
        if (auto version = parseVersion(releaseVersions.value(versionGameVersion), modId)) {
            info.mReleases[*index].mGameVersions.append(std::move(version).value());
        }
    }

    qCDebug(cModInfoCache, "Read details of %s from the cache", qUtf8Printable(modId));
    return info;
}

void ModInfoCache::setModInfo(const QString &modId, const ModOnlineInfo &modInfo) {
    if (!getQuery(QueryType::InsertDetails)) {
        return;
    }

    const auto fail = [this, &modId](const QSqlQuery &failed) {
        qCWarning(cModInfoCache, "Cannot store details of %s: %s", qUtf8Printable(modId),
                  qUtf8Printable(failed.lastError().text()));
        mDatabase.rollback();
    };

    if (!mDatabase.transaction()) {
        qCWarning(cModInfoCache, "Cannot start storing %s: %s", qUtf8Printable(modId),
                  qUtf8Printable(mDatabase.lastError().text()));
        return;
    }

    QSqlQuery &deleteDetails = *getQuery(QueryType::DeleteDetails);
    deleteDetails.bindValue(u":mod_id"_s, modId);
    if (!deleteDetails.exec()) {
        return fail(deleteDetails);
    }

    QSqlQuery &details = *getQuery(QueryType::InsertDetails);
    details.bindValue(u":mod_id"_s, modId);
    details.bindValue(u":modid"_s, modInfo.mModId);
    details.bindValue(u":assetid"_s, modInfo.mAssetId);
    details.bindValue(u":name"_s, modInfo.mName);
    details.bindValue(u":author"_s, modInfo.mAuthor);
    details.bindValue(u":urlalias"_s, modInfo.mUrlAlias);
    details.bindValue(u":side"_s, modInfo.mSide);
    details.bindValue(u":type"_s, modInfo.mType);
    details.bindValue(u":text"_s, modInfo.mText);
    details.bindValue(u":logofile"_s, modInfo.mLogo.toString());
    details.bindValue(u":homepageurl"_s, modInfo.mHomepage.toString());
    details.bindValue(u":sourcecodeurl"_s, modInfo.mSourceCode.toString());
    details.bindValue(u":issuetrackerurl"_s, modInfo.mIssueTracker.toString());
    details.bindValue(u":wikiurl"_s, modInfo.mWiki.toString());
    details.bindValue(u":trailervideourl"_s, modInfo.mTrailerVideo.toString());
    details.bindValue(u":downloads"_s, modInfo.mDownloads);
    details.bindValue(u":follows"_s, modInfo.mFollows);
    details.bindValue(u":comments"_s, modInfo.mComments);
    details.bindValue(u":trendingpoints"_s, modInfo.mTrendingPoints);
    details.bindValue(u":created"_s, epochOrNull(modInfo.mCreated));
    details.bindValue(u":lastmodified"_s, epochOrNull(modInfo.mLastModified));
    details.bindValue(u":lastreleased"_s, epochOrNull(modInfo.mLastReleased));
    details.bindValue(u":fetched_at"_s, QDateTime::currentSecsSinceEpoch());
    if (!details.exec()) {
        return fail(details);
    }

    QSqlQuery &tag = *getQuery(QueryType::InsertTag);
    for (qsizetype position = 0; position < modInfo.mTags.size(); ++position) {
        tag.bindValue(u":mod_id"_s, modId);
        tag.bindValue(u":position"_s, position);
        tag.bindValue(u":tag"_s, modInfo.mTags.at(position));
        if (!tag.exec()) {
            return fail(tag);
        }
    }

    QSqlQuery &screenshot = *getQuery(QueryType::InsertScreenshot);
    for (qsizetype position = 0; position < modInfo.mScreenshots.size(); ++position) {
        const ModScreenshot &shot = modInfo.mScreenshots.at(position);
        screenshot.bindValue(u":mod_id"_s, modId);
        screenshot.bindValue(u":position"_s, position);
        screenshot.bindValue(u":fileid"_s, shot.mFileId);
        screenshot.bindValue(u":mainfile"_s, shot.mMainFile.toString());
        screenshot.bindValue(u":filename"_s, shot.mFileName);
        screenshot.bindValue(u":thumbnailfilename"_s, shot.mThumbnail.toString());
        screenshot.bindValue(u":created"_s, epochOrNull(shot.mCreated));
        if (!screenshot.exec()) {
            return fail(screenshot);
        }
    }

    QSqlQuery &release = *getQuery(QueryType::InsertRelease);
    QSqlQuery &releaseVersion = *getQuery(QueryType::InsertReleaseVersion);
    for (qsizetype position = 0; position < modInfo.mReleases.size(); ++position) {
        const ModRelease &modRelease = modInfo.mReleases.at(position);
        release.bindValue(u":mod_id"_s, modId);
        release.bindValue(u":position"_s, position);
        release.bindValue(u":releaseid"_s, modRelease.mReleaseId);
        release.bindValue(u":modidstr"_s, modRelease.mModIdStr);
        release.bindValue(u":modversion"_s, versionText(modRelease.mModVersion));
        release.bindValue(u":filename"_s, modRelease.mFileName);
        release.bindValue(u":mainfile"_s, modRelease.mMainFile.toString());
        release.bindValue(u":fileid"_s, modRelease.mFileId);
        release.bindValue(u":downloads"_s, modRelease.mDownloads);
        release.bindValue(u":created"_s, epochOrNull(modRelease.mCreated));
        release.bindValue(u":changelog"_s, modRelease.mChangelog);
        if (!release.exec()) {
            return fail(release);
        }

        for (qsizetype versionPosition = 0; versionPosition < modRelease.mGameVersions.size(); ++versionPosition) {
            releaseVersion.bindValue(u":mod_id"_s, modId);
            releaseVersion.bindValue(u":release_position"_s, position);
            releaseVersion.bindValue(u":position"_s, versionPosition);
            releaseVersion.bindValue(u":gameversion"_s, versionText(modRelease.mGameVersions.at(versionPosition)));
            if (!releaseVersion.exec()) {
                return fail(releaseVersion);
            }
        }
    }

    if (!mDatabase.commit()) {
        qCWarning(cModInfoCache, "Cannot commit details of %s: %s", qUtf8Printable(modId),
                  qUtf8Printable(mDatabase.lastError().text()));
        mDatabase.rollback();
        return;
    }
    qCDebug(cModInfoCache, "Stored details of %s with %lld releases", qUtf8Printable(modId),
            static_cast<long long>(modInfo.mReleases.size()));
}

void ModInfoCache::storeModList(QByteArray modList) {
    if (!getQuery(QueryType::Details)) {
        QMetaObject::invokeMethod(this, [this] { emit modListStored(false); }, Qt::QueuedConnection);
        return;
    }

    mWriter.start([this, dbPath = mDbLocation, modList = std::move(modList)] {
        const auto entries = parseModList(modList);
        const bool stored = entries && writeModList(WORKER_CONNECTION_NAME, dbPath, *entries);
        QSqlDatabase::removeDatabase(WORKER_CONNECTION_NAME);
        QMetaObject::invokeMethod(this, [this, stored] { emit modListStored(stored); }, Qt::QueuedConnection);
    });
}

bool ModInfoCache::resetSchema(QSqlQuery &query) {
    const auto fail = [this, &query] {
        qCWarning(cModInfoCache, "Cannot reset the schema of %s: %s", qUtf8Printable(mDbLocation),
                  qUtf8Printable(query.lastError().text()));
        mDatabase.rollback();
        return false;
    };

    if (!query.exec(u"SELECT name FROM sqlite_schema WHERE type = 'table' AND name NOT LIKE 'sqlite_%'"_s)) {
        return fail();
    }
    QStringList tables;
    while (query.next()) {
        tables.append(query.value(0).toString());
    }
    query.finish();

    if (!mDatabase.transaction()) {
        return fail();
    }

    for (const QString &table : tables) {
        if (!query.exec(u"DROP TABLE %1"_s.arg(table))) {
            return fail();
        }
    }
    for (const QLatin1StringView statement : SCHEMA) {
        if (!query.exec(QString{statement})) {
            return fail();
        }
    }
    if (!query.exec(u"PRAGMA user_version = %1"_s.arg(CACHE_VERSION))) {
        return fail();
    }
    if (!mDatabase.commit()) {
        return fail();
    }
    qCInfo(cModInfoCache, "Created schema %d in %s", CACHE_VERSION, qUtf8Printable(mDbLocation));
    return true;
}

bool ModInfoCache::createStatements() {
    static_assert(QUERIES.size() == std::to_underlying(QueryType::Count));

    for (std::size_t i = 0; i < mStatements.size(); i++) {
        mStatements[i] = std::make_unique<QSqlQuery>(mDatabase);
        if (!mStatements[i]->prepare(QUERIES[i])) {
            qCWarning(cModInfoCache, "Cannot prepare query %zu: %s", i,
                      qUtf8Printable(mStatements[i]->lastError().text()));
            mStatements = {};
            return false;
        }
    }

    return true;
}

const std::unique_ptr<QSqlQuery> &ModInfoCache::getQuery(QueryType queryType) const {
    return mStatements[static_cast<std::size_t>(queryType)];
}
} // namespace vsmm