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

#include <ModEntryTestUtils.hpp>
#include <ModInfoCache.hpp>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTest>
#include <QTimeZone>

#include <chrono>
#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

namespace {

// the file ModInfoCache opens, under the test mode cache dir
[[nodiscard]] QString dbPath() {
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + u"/modinfocache.db"_s;
}

[[nodiscard]] QDateTime utc(QDate date, QTime time) { return QDateTime{date, time, QTimeZone::UTC}; }

// for rows the cache api cannot write (old fetch times, an older schema), returns the first error or an empty string
[[nodiscard]] QString execRaw(const QStringList &statements) {
    const QString connection = u"modinfocache-test"_s;
    QString error;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, connection);
        db.setDatabaseName(dbPath());
        if (!db.open()) {
            error = db.lastError().text();
        }
        QSqlQuery query{db};
        for (const QString &statement : statements) {
            if (!error.isEmpty()) {
                break;
            }
            if (!query.exec(statement)) {
                error = u"%1: %2"_s.arg(statement, query.lastError().text());
            }
        }
    }
    QSqlDatabase::removeDatabase(connection);
    return error;
}

// first column of the first row, or an invalid QVariant
[[nodiscard]] QVariant queryRaw(const QString &statement) {
    const QString connection = u"modinfocache-test"_s;
    QVariant value;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, connection);
        db.setDatabaseName(dbPath());
        QSqlQuery query{db};
        if (db.open() && query.exec(statement) && query.next()) {
            value = query.value(0);
        }
    }
    QSqlDatabase::removeDatabase(connection);
    return value;
}

[[nodiscard]] vsmm::ModRelease release(const QString &version, const QStringList &gameVersions, qint64 releaseId) {
    vsmm::ModRelease release;
    release.mReleaseId = releaseId;
    release.mFileId = releaseId * 2;
    release.mDownloads = 2394;
    release.mModIdStr = u"carryon"_s;
    release.mFileName = u"CarryOn_v%1.zip"_s.arg(version);
    release.mChangelog = u"<p>changes in %1</p>"_s.arg(version);
    release.mModVersion = ver(version);
    release.mCreated = utc({2026, 10, 14}, {13, 43, 4});
    release.mMainFile = QUrl{u"https://moddbcdn.vintagestory.at/CarryOn_v%1.zip"_s.arg(version)};
    for (const QString &gameVersion : gameVersions) {
        release.mGameVersions.append(ver(gameVersion));
    }
    return release;
}

// every field set, so a column that is not written or not read shows up as a difference
[[nodiscard]] vsmm::ModOnlineInfo fullInfo(const QDateTime &lastReleased = utc({2026, 10, 14}, {14, 48, 50})) {
    vsmm::ModScreenshot screenshot;
    screenshot.mFileId = 8045;
    screenshot.mMainFile = QUrl{u"https://moddbcdn.vintagestory.at/CarryOnLogo2.png"_s};
    screenshot.mThumbnail = QUrl{u"https://moddbcdn.vintagestory.at/CarryOnLogo2_55_60.png"_s};
    screenshot.mFileName = u"CarryOnLogo2.png"_s;
    // inside the central european dst gap, so a local time conversion would move it
    screenshot.mCreated = utc({2026, 3, 29}, {2, 30, 0});

    vsmm::ModOnlineInfo info;
    info.mModId = 890;
    info.mAssetId = 4405;
    info.mDownloads = 1097804;
    info.mFollows = 8001;
    info.mComments = 769;
    info.mTrendingPoints = -1;
    info.mName = u"Carry On"_s;
    info.mAuthor = u"NerdScurvy"_s;
    info.mUrlAlias = u"carryon"_s;
    info.mSide = u"both"_s;
    info.mType = u"mod"_s;
    info.mText = u"<p>Pick up chests</p>"_s;
    info.mCreated = utc({2022, 10, 22}, {1, 30, 37});
    info.mLastModified = utc({2026, 10, 21}, {17, 55, 19});
    info.mLastReleased = lastReleased;
    info.mLogo = QUrl{u"https://moddbcdn.vintagestory.at/CarryOnLogo.png"_s};
    info.mHomepage = QUrl{u"https://example.org/carryon"_s};
    info.mSourceCode = QUrl{u"https://github.com/NerdScurvy/CarryOn"_s};
    info.mIssueTracker = QUrl{u"https://github.com/NerdScurvy/CarryOn/issues"_s};
    info.mWiki = QUrl{u"https://wiki.vintagestory.at/CarryOn"_s};
    info.mTrailerVideo = QUrl{u"https://www.youtube.com/watch?v=carryon"_s};
    info.mTags = {u"Storage"_s, u"QoL"_s};
    // api order, not version order, and build metadata that semver's == ignores
    info.mReleases = {release(u"2.0.0-rc.1+build.7"_s, {u"1.22.0"_s, u"1.22.1"_s}, 61180),
                      release(u"1.14.3"_s, {u"1.21.0"_s}, 60001)};
    info.mScreenshots = {screenshot};
    return info;
}

[[nodiscard]] QJsonObject listEntry(qint64 modId, const QStringList &ids, const QJsonValue &lastReleased) {
    QJsonObject entry{{"modid"_L1, modId},
                      {"assetid"_L1, modId * 5},
                      {"name"_L1, u"Mod %1"_s.arg(modId)},
                      {"modidstrs"_L1, QJsonArray::fromStringList(ids)},
                      {"tags"_L1, QJsonArray{"Storage"_L1, "QoL"_L1}}};
    if (!lastReleased.isUndefined()) {
        entry.insert("lastreleased"_L1, lastReleased);
    }
    return entry;
}

[[nodiscard]] QByteArray listReply(const QJsonArray &mods) {
    return QJsonDocument{QJsonObject{{"statuscode"_L1, "200"_L1}, {"mods"_L1, mods}}}.toJson(QJsonDocument::Compact);
}

constexpr auto CARRYON_RELEASED = "2026-10-14 14:48:50";

} // namespace

class ModInfoCacheUnitTest : public QObject {
    Q_OBJECT

    std::unique_ptr<vsmm::ModInfoCache> mCache;

    // the connection name is fixed, so an instance is only created once the previous one is gone
    void reopen() {
        mCache.reset();
        mCache = std::make_unique<vsmm::ModInfoCache>();
    }

    static void removeDatabaseFiles() {
        for (const QString &suffix : {u""_s, u"-wal"_s, u"-shm"_s}) {
            QFile::remove(dbPath() + suffix);
        }
    }

    // hands a reply to the worker and waits for its answer, check QTest::currentTestFailed() after it
    void storeList(const QByteArray &reply, bool expectedStored) {
        QSignalSpy stored{mCache.get(), &vsmm::IModInfoCache::modListStored};
        mCache->storeModList(reply);
        QTRY_COMPARE(stored.count(), 1);
        QCOMPARE(stored.at(0).at(0).toBool(), expectedStored);
    }

  private slots:
    void initTestCase() {
        QLoggingCategory::setFilterRules(u"*=false\nmodinfocache=true\nmodinfocache.info=false\nmodinfocache.debug=false"_s);
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(dbPath().contains(".qttest"_L1), qPrintable(dbPath()));
    }

    void cleanupTestCase() { QStandardPaths::setTestModeEnabled(false); }

    void init() {
        removeDatabaseFiles();
        reopen();
    }

    void cleanup() {
        mCache.reset();
        removeDatabaseFiles();
    }

    // details

    void emptyCacheMisses() { QVERIFY(!mCache->getModInfo(u"carryon"_s)); }

    void storedDetailsReadBackEqual() {
        const vsmm::ModOnlineInfo info = fullInfo();

        mCache->setModInfo(u"carryon"_s, info);
        const auto stored = mCache->getModInfo(u"carryon"_s);

        QVERIFY(stored);
        // separate compares first, a failing ModOnlineInfo compare cannot print which field differs
        QVERIFY(stored->mScreenshots == info.mScreenshots);
        QVERIFY(stored->mReleases == info.mReleases);
        QVERIFY(*stored == info);
        // semver's == ignores build metadata, the text does not
        QCOMPARE(str(stored->mReleases.at(0).mModVersion), u"2.0.0-rc.1+build.7"_s);
        QCOMPARE(str(stored->mReleases.at(0).mGameVersions.at(1)), u"1.22.1"_s);
        QCOMPARE(stored->mScreenshots.at(0).mCreated.timeSpec(), Qt::UTC);
    }

    void secondStoreReplacesTheChildRows() {
        mCache->setModInfo(u"carryon"_s, fullInfo());
        vsmm::ModOnlineInfo smaller = fullInfo();
        smaller.mReleases = {release(u"1.14.3"_s, {u"1.21.0"_s}, 60001)};
        smaller.mTags = {u"QoL"_s};
        smaller.mScreenshots.clear();

        mCache->setModInfo(u"carryon"_s, smaller);

        const auto stored = mCache->getModInfo(u"carryon"_s);
        QVERIFY(stored);
        QVERIFY(*stored == smaller);
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_releases"_s).toInt(), 1);
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_release_versions"_s).toInt(), 1);
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_detail_tags"_s).toInt(), 1);
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_screenshots"_s).toInt(), 0);
    }

    void detailsSurviveAReopen() {
        mCache->setModInfo(u"carryon"_s, fullInfo());

        reopen();

        const auto stored = mCache->getModInfo(u"carryon"_s);
        QVERIFY(stored);
        QVERIFY(*stored == fullInfo());
    }

    // modinfo ids and api ids do not always agree on case
    void idCaseDoesNotMatter() {
        mCache->setModInfo(u"carryon"_s, fullInfo());

        QVERIFY(mCache->getModInfo(u"CarryOn"_s));
    }

    void otherSchemaVersionRebuildsTheDatabase() {
        mCache->setModInfo(u"carryon"_s, fullInfo());
        mCache.reset();
        QCOMPARE(execRaw({u"PRAGMA user_version = 0"_s, u"CREATE TABLE old_table (x INTEGER)"_s}), QString{});

        reopen();

        QVERIFY(!mCache->getModInfo(u"carryon"_s));
        QCOMPARE(queryRaw(u"SELECT count(*) FROM sqlite_schema WHERE name = 'old_table'"_s).toInt(), 0);
        mCache->setModInfo(u"carryon"_s, fullInfo());
        QVERIFY(mCache->getModInfo(u"carryon"_s));
    }

    void oldDetailsArePrunedOnOpen() {
        mCache->setModInfo(u"carryon"_s, fullInfo());
        mCache->setModInfo(u"betterruins"_s, fullInfo());
        mCache.reset();
        const qint64 oldFetch = QDateTime::currentSecsSinceEpoch() - std::chrono::seconds{std::chrono::days{31}}.count();
        QCOMPARE(execRaw({u"UPDATE mod_details SET fetched_at = %1 WHERE mod_id = 'carryon'"_s.arg(oldFetch)}),
                 QString{});

        reopen();

        QVERIFY(!mCache->getModInfo(u"carryon"_s));
        QVERIFY(mCache->getModInfo(u"betterruins"_s));
        // the cascade takes the child rows with it
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_releases WHERE mod_id = 'carryon'"_s).toInt(), 0);
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_release_versions WHERE mod_id = 'carryon'"_s).toInt(), 0);
    }

    void unopenableDatabaseActsAsEmpty() {
        mCache.reset();
        removeDatabaseFiles();
        // a directory where the file should be, so sqlite cannot open it
        QVERIFY(QDir{}.mkpath(dbPath()));
        const auto removeDirectory = qScopeGuard([] { QDir{dbPath()}.removeRecursively(); });

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression{u"^Failed to open database .*modinfocache\\.db: "_s});
        reopen();

        mCache->setModInfo(u"carryon"_s, fullInfo());
        QVERIFY(!mCache->getModInfo(u"carryon"_s));
        storeList(listReply({listEntry(890, {u"carryon"_s}, QJsonValue{CARRYON_RELEASED})}), false);
    }

    // list

    void storedListFillsTheTables() {
        storeList(listReply({listEntry(890, {u"carryon"_s, u"carryonextra"_s}, QJsonValue{CARRYON_RELEASED}),
                             listEntry(12, {u"betterruins"_s}, QJsonValue{"2026-09-30 10:00:00"_L1})}),
                  true);
        if (QTest::currentTestFailed()) {
            return;
        }

        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_list"_s).toInt(), 2);
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_list_ids"_s).toInt(), 3);
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_list_tags"_s).toInt(), 4);
        QCOMPARE(queryRaw(u"SELECT modid FROM mod_list_ids WHERE mod_id = 'carryonextra'"_s).toLongLong(), 890);
    }

    void newListReplacesTheStoredOne() {
        storeList(listReply({listEntry(1, {u"first"_s}, QJsonValue{CARRYON_RELEASED}),
                             listEntry(2, {u"second"_s}, QJsonValue{CARRYON_RELEASED})}),
                  true);
        if (QTest::currentTestFailed()) {
            return;
        }

        storeList(listReply({listEntry(2, {u"second"_s}, QJsonValue{CARRYON_RELEASED}),
                             listEntry(3, {u"third"_s}, QJsonValue{CARRYON_RELEASED})}),
                  true);
        if (QTest::currentTestFailed()) {
            return;
        }

        QCOMPARE(queryRaw(u"SELECT group_concat(modid) FROM (SELECT modid FROM mod_list ORDER BY modid)"_s).toString(),
                 u"2,3"_s);
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_list_ids WHERE mod_id = 'first'"_s).toInt(), 0);
    }

    void unusableListKeepsTheStoredOne_data() {
        QTest::addColumn<QByteArray>("reply");
        QTest::addColumn<QStringList>("warnings");
        QTest::newRow("not-json") << QByteArray{"{broken"} << QStringList{u"^Cannot parse the mod list: "_s};
        QTest::newRow("status-404") << QByteArray{R"({"statuscode":"404"})"}
                                    << QStringList{u"^Mod list reply has status 404 and no mods array$"_s};
        QTest::newRow("no-mods-array") << QByteArray{R"({"statuscode":"200"})"}
                                       << QStringList{u"^Mod list reply has status 200 and no mods array$"_s};
        QTest::newRow("empty-mods") << listReply({})
                                    << QStringList{u"^Mod list reply has no mods, keeping the stored list$"_s};
        QTest::newRow("no-usable-mods") << listReply({QJsonObject{{"name"_L1, "Broken"_L1}}})
                                        << QStringList{u"^Skipped mod list entry Broken without modid$"_s,
                                                       u"^Mod list reply has no usable mods, keeping the stored list$"_s};
    }

    void unusableListKeepsTheStoredOne() {
        QFETCH(QByteArray, reply);
        QFETCH(QStringList, warnings);
        storeList(listReply({listEntry(890, {u"carryon"_s}, QJsonValue{CARRYON_RELEASED})}), true);
        if (QTest::currentTestFailed()) {
            return;
        }

        for (const QString &warning : warnings) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression{warning});
        }
        storeList(reply, false);
        if (QTest::currentTestFailed()) {
            return;
        }

        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_list"_s).toInt(), 1);
        QCOMPARE(queryRaw(u"SELECT modid FROM mod_list_ids WHERE mod_id = 'carryon'"_s).toLongLong(), 890);
    }

    void entryWithoutModIdIsSkipped() {
        QTest::ignoreMessage(QtWarningMsg, "Skipped mod list entry Broken without modid");

        storeList(listReply({QJsonObject{{"name"_L1, "Broken"_L1}},
                             listEntry(890, {u"carryon"_s}, QJsonValue{CARRYON_RELEASED})}),
                  true);
        if (QTest::currentTestFailed()) {
            return;
        }

        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_list"_s).toInt(), 1);
    }

    // one string id on two mod pages would break the primary key, so the first page wins
    void sharedModIdKeepsTheFirstMod() {
        storeList(listReply({listEntry(1, {u"shared"_s}, QJsonValue{CARRYON_RELEASED}),
                             listEntry(2, {u"shared"_s, u"other"_s}, QJsonValue{CARRYON_RELEASED})}),
                  true);
        if (QTest::currentTestFailed()) {
            return;
        }

        QCOMPARE(queryRaw(u"SELECT modid FROM mod_list_ids WHERE mod_id = 'shared'"_s).toLongLong(), 1);
        QCOMPARE(queryRaw(u"SELECT count(*) FROM mod_list"_s).toInt(), 2);
    }

    // details against the list

    void detailsHitWhenTheListLacksTheMod() {
        storeList(listReply({listEntry(12, {u"betterruins"_s}, QJsonValue{"2026-09-30 10:00:00"_L1})}), true);
        if (QTest::currentTestFailed()) {
            return;
        }

        mCache->setModInfo(u"carryon"_s, fullInfo());

        QVERIFY(mCache->getModInfo(u"carryon"_s));
    }

    void detailsFollowTheListedLastReleased_data() {
        QTest::addColumn<QString>("listedId");
        QTest::addColumn<QJsonValue>("listed");
        QTest::addColumn<QDateTime>("stored");
        QTest::addColumn<bool>("hit");
        const QDateTime released = utc({2026, 10, 14}, {14, 48, 50});
        QTest::newRow("same") << u"carryon"_s << QJsonValue{CARRYON_RELEASED} << released << true;
        // a release or an edit of its game versions since the details were fetched
        QTest::newRow("newer-in-list") << u"carryon"_s << QJsonValue{"2026-11-07 19:02:11"_L1} << released << false;
        QTest::newRow("second-id-of-the-page")
            << u"carryonextra"_s << QJsonValue{"2026-11-07 19:02:11"_L1} << released << false;
        // NULL = NULL is not true in sql, so a missing date never counts as fresh
        QTest::newRow("both-dates-missing") << u"carryon"_s << QJsonValue{QJsonValue::Undefined} << QDateTime{}
                                            << false;
        QTest::newRow("stored-date-missing") << u"carryon"_s << QJsonValue{CARRYON_RELEASED} << QDateTime{} << false;
    }

    void detailsFollowTheListedLastReleased() {
        QFETCH(QString, listedId);
        QFETCH(QJsonValue, listed);
        QFETCH(QDateTime, stored);
        QFETCH(bool, hit);
        storeList(listReply({listEntry(890, {u"carryon"_s, u"carryonextra"_s}, listed)}), true);
        if (QTest::currentTestFailed()) {
            return;
        }

        mCache->setModInfo(listedId, fullInfo(stored));

        QCOMPARE(mCache->getModInfo(listedId).has_value(), hit);
    }

    // a select left open keeps the old snapshot, and a write on it fails once the worker has committed
    void writeAfterANewListSucceeds() {
        storeList(listReply({listEntry(890, {u"carryon"_s}, QJsonValue{CARRYON_RELEASED})}), true);
        if (QTest::currentTestFailed()) {
            return;
        }
        mCache->setModInfo(u"carryon"_s, fullInfo());
        QVERIFY(mCache->getModInfo(u"carryon"_s));

        storeList(listReply({listEntry(890, {u"carryon"_s}, QJsonValue{"2026-11-07 19:02:11"_L1})}), true);
        if (QTest::currentTestFailed()) {
            return;
        }

        // written before any read, so nothing has reset the statements of the read above
        mCache->setModInfo(u"carryon"_s, fullInfo(utc({2026, 11, 7}, {19, 2, 11})));
        const auto stored = mCache->getModInfo(u"carryon"_s);
        QVERIFY(stored);
        QCOMPARE(stored->mLastReleased, utc({2026, 11, 7}, {19, 2, 11}));
    }
};

QTEST_GUILESS_MAIN(ModInfoCacheUnitTest)
#include "ModInfoCacheUnitTest.moc"
