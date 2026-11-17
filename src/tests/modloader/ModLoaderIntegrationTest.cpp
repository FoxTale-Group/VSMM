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

#include <HttpClientTestUtils.hpp>
#include <ModEntryTestUtils.hpp>
#include <ModInfoCache.hpp>
#include <ModLoader.hpp>
#include <RealComponents.hpp>
#include <ZipTestUtils.hpp>

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTest>
#include <QUuid>

#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

namespace {
constexpr auto RELEASED = "2026-10-14 14:48:50"_L1;

[[nodiscard]] QByteArray modInfo(const QString &id, const QString &version) {
    const QJsonObject info{{"modid"_L1, id},
                           {"name"_L1, u"%1 (local)"_s.arg(id)},
                           {"authors"_L1, QJsonArray{"local author"_L1}},
                           {"version"_L1, version}};
    return QJsonDocument{info}.toJson(QJsonDocument::Compact);
}

// the status lives in the body, the transport status is 200 either way
[[nodiscard]] Response info(QJsonObject mod) {
    // the same release as the list, so the stored details stay fresh
    mod["lastreleased"_L1] = RELEASED;
    const QJsonObject body{{"statuscode"_L1, "200"_L1}, {"mod"_L1, mod}};
    return {.mBody = QJsonDocument{body}.toJson(QJsonDocument::Compact)};
}

// /api/mods with every mod the tests install
[[nodiscard]] Response modList(const QString &released = RELEASED) {
    QJsonArray mods;
    for (const auto &[modId, id] : {std::pair{1, "carryon"_L1}, std::pair{2, "betterruins"_L1}}) {
        mods.append(QJsonObject{{"modid"_L1, modId}, {"modidstrs"_L1, QJsonArray{id}}, {"lastreleased"_L1, released}});
    }
    const QJsonObject body{{"statuscode"_L1, "200"_L1}, {"mods"_L1, mods}};
    return {.mBody = QJsonDocument{body}.toJson(QJsonDocument::Compact)};
}

[[nodiscard]] Response download(QByteArray zip) { return {.mContentType = "application/zip", .mBody = std::move(zip)}; }

[[nodiscard]] QString cacheDir() { return QStandardPaths::writableLocation(QStandardPaths::CacheLocation); }

// every test starts without stored details
void removeDatabaseFiles() {
    for (const auto suffix : {""_L1, "-wal"_L1, "-shm"_L1}) {
        QFile::remove(cacheDir() + "/modinfocache.db"_L1 + suffix);
    }
}

[[nodiscard]] QString tempPath(const QString &fileName) {
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + fileName;
}

// empty when the zip could not be written
[[nodiscard]] QString writeMod(const QDir &dir, const QString &id, const QString &version) {
    const QString path = dir.absoluteFilePath(u"%1-%2.zip"_s.arg(id, version));
    const QList<Entry> entries{{"modinfo.json", modInfo(id, version)}};
    if (!QDir{}.mkpath(dir.absolutePath()) || !writeArchive(path, entries)) {
        return {};
    }
    return path;
}
} // namespace

class ModLoaderIntegrationTest : public QObject {
    Q_OBJECT

    // loader and store warnings stay visible, their info chatter is pinned by the unit suites
    static void onlyWarnings() {
        QLoggingCategory::setFilterRules(u"*=false\nmodloader=true\nmodloader.info=false\nmodloader.debug=false\n"
                                         u"modstore=true\nmodstore.info=false\nmodstore.debug=false"_s);
    }

    std::unique_ptr<FakeHttpServer> mServer;
    std::unique_ptr<vsmm::HttpClient> mHttp;
    std::unique_ptr<vsmm::ModInfoCache> mCache;
    std::unique_ptr<RealComponents> mApp;
    std::unique_ptr<vsmm::ModLoader> mLoader;
    QString mUpdateFileName;

    [[nodiscard]] vsmm::ModStore &store() const { return mApp->store(); }
    [[nodiscard]] const vsmm::ModEntry *mod(const QString &id) const { return store().find(id); }

    // the store clears it when the loader reports back
    [[nodiscard]] bool settled() const { return !store().isWorkPending(); }

    [[nodiscard]] bool hasOnlineInfo(const QString &id) const { return mod(id) && !mod(id)->getTags().isEmpty(); }

    // same order as App
    void connectLoader() {
        mLoader = std::make_unique<vsmm::ModLoader>();
        mLoader->setApiUrl(mServer->url("api/"_L1));
        mLoader->setHttpClient(mHttp.get());
        mLoader->setInfoCache(mCache.get());
        mLoader->setStore(&mApp->store());
        mLoader->setGameMngr(&mApp->gameMngr());
    }

    // from skips the requests of an earlier session
    [[nodiscard]] QStringList requestedPaths(qsizetype from = 0) const {
        QStringList paths;
        for (const auto &request : mServer->requests().sliced(from)) {
            paths.append(QString::fromUtf8(request.mTarget));
        }
        paths.sort();
        return paths;
    }

    [[nodiscard]] QString downloadPath() const { return u"download/%1"_s.arg(mUpdateFileName); }

    // supports the fake game version, downloads from the fake server
    [[nodiscard]] QJsonObject newerRelease() const {
        QJsonObject newer = release(u"1.1.0"_s, {u"1.22.0"_s});
        newer["filename"_L1] = mUpdateFileName;
        newer["mainfile"_L1] = mServer->url(QLatin1StringView{downloadPath().toLatin1()}).toString();
        return newer;
    }

  private slots:
    void initTestCase() {
        // the store needs a game version, which comes from a fake game exe, a POSIX shell script
        SKIP_WITHOUT_POSIX_SHELL();
        QCoreApplication::setOrganizationName(RealComponents::TEST_ORG_NAME);
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(cacheDir().contains("qttest"_L1), qPrintable(cacheDir()));
        QVERIFY(QDir{cacheDir()}.removeRecursively());
    }

    void cleanupTestCase() {
        QDir{cacheDir()}.removeRecursively();
        QStandardPaths::setTestModeEnabled(false);
    }

    void init() {
        onlyWarnings();
        mUpdateFileName = u"vsmm-modloader-%1.zip"_s.arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        mServer = std::make_unique<FakeHttpServer>();
        QVERIFY(mServer->start());
        mServer->route("/api/mods", modList());
        mHttp = std::make_unique<vsmm::HttpClient>();
        removeDatabaseFiles();
        mCache = std::make_unique<vsmm::ModInfoCache>();
        mApp = std::make_unique<RealComponents>();
        mApp->beforeValidate([this] { connectLoader(); });
    }

    void cleanup() {
        // waits for the worker pools, and goes before the store it points at
        mLoader.reset();
        mApp.reset();
        mCache.reset();
        removeDatabaseFiles();
        mHttp.reset();
        mServer.reset();
        QFile::remove(tempPath(mUpdateFileName));
    }

    void startupScanLoadsInstalledMods() {
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        mServer->setFallback(info(modJson()));

        QVERIFY(mApp->start());

        QTRY_VERIFY(settled());
        QVERIFY(hasOnlineInfo(u"carryon"_s));
        QCOMPARE(mod(u"carryon"_s)->getVersion(), ver(u"1.0.0"_s));
        QCOMPARE(requestedPaths(), (QStringList{u"/api/mod/carryon"_s, u"/api/mods"_s}));
    }

    void startupScanCoversEveryModsDir() {
        RealComponents::Options options;
        options.mExtraModsDirs = {u"extra"_s};
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        QVERIFY(!writeMod(mApp->dir(u"extra"_s), u"betterruins"_s, u"1.0.0"_s).isEmpty());
        mServer->setFallback(info(modJson()));

        QVERIFY(mApp->start(options));

        QTRY_VERIFY(settled());
        QVERIFY(hasOnlineInfo(u"carryon"_s));
        QVERIFY(hasOnlineInfo(u"betterruins"_s));
        QCOMPARE(requestedPaths(), (QStringList{u"/api/mod/betterruins"_s, u"/api/mod/carryon"_s, u"/api/mods"_s}));
    }

    void modUnknownToTheApiKeepsItsLocalInfo() {
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        mServer->setFallback({.mBody = R"({"statuscode":"404"})"});

        QVERIFY(mApp->start());

        QTRY_VERIFY(settled());
        QVERIFY(mod(u"carryon"_s));
        QVERIFY(!hasOnlineInfo(u"carryon"_s));
    }

    void failedInfoRequestKeepsTheLocalInfo() {
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        mServer->setFallback({.mStatus = 404});

        QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression{
                u"^Error retrieving info for carryon: Error receiving response\\. .+ HTTP Status: 404$"_s});
        QVERIFY(mApp->start());

        QTRY_VERIFY(settled());
        QVERIFY(mod(u"carryon"_s));
        QVERIFY(!hasOnlineInfo(u"carryon"_s));
        // a 404 is not retried
        QCOMPARE(requestedPaths(), (QStringList{u"/api/mod/carryon"_s, u"/api/mods"_s}));
    }

    void reloadRescansTheModsDir() {
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        mServer->setFallback(info(modJson()));
        QVERIFY(mApp->start());
        QTRY_VERIFY(settled());

        QVERIFY(!writeMod(mApp->modsDir(), u"betterruins"_s, u"1.0.0"_s).isEmpty());
        emit mApp->gameMngr().modsDirsChanged();
        QVERIFY(store().isWorkPending());

        QTRY_VERIFY(settled());
        QVERIFY(hasOnlineInfo(u"carryon"_s));
        QVERIFY(hasOnlineInfo(u"betterruins"_s));
        // carryon is stored by then, and the loader downloads the list once per session
        QCOMPARE(requestedPaths(), (QStringList{u"/api/mod/betterruins"_s, u"/api/mod/carryon"_s, u"/api/mods"_s}));
    }

    void modPickedInTheGuiIsCopiedIntoTheModsDir() {
        mServer->setFallback(info(modJson()));
        QVERIFY(mApp->start());
        QTRY_VERIFY(settled());
        const QString picked = writeMod(mApp->stagingDir(), u"carryon"_s, u"1.0.0"_s);
        QVERIFY(!picked.isEmpty());

        mLoader->load(QUrl::fromLocalFile(picked));

        QTRY_VERIFY(hasOnlineInfo(u"carryon"_s));
        const QString installed = mApp->modsDir().absoluteFilePath(u"carryon-1.0.0.zip"_s);
        QCOMPARE(mod(u"carryon"_s)->getFileInfo().absoluteFilePath(), installed);
        QCOMPARE(readFile(installed), readFile(picked));
    }

    void updateReplacesTheInstalledVersion() {
        const QString installed = writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s);
        QVERIFY(!installed.isEmpty());
        const QByteArray zip = readFile(writeMod(mApp->stagingDir(), u"carryon"_s, u"1.1.0"_s));
        QVERIFY(!zip.isEmpty());
        const QJsonObject online = modJson({newerRelease()});
        mServer->enqueue(info(online));
        mServer->enqueue(download(zip));
        mServer->setFallback(info(online));
        QVERIFY(mApp->start());
        QTRY_VERIFY(settled());
        QVERIFY(mod(u"carryon"_s)->hasUpdate());

        store().update(u"carryon"_s);
        QVERIFY(store().isWorkPending());

        QTRY_COMPARE(mod(u"carryon"_s)->getVersion(), ver(u"1.1.0"_s));
        QTRY_VERIFY(settled());
        QVERIFY(!mod(u"carryon"_s)->hasUpdate());
        const QString updated = mApp->modsDir().absoluteFilePath(mUpdateFileName);
        QCOMPARE(mod(u"carryon"_s)->getFileInfo().absoluteFilePath(), updated);
        QCOMPARE(readFile(updated), zip);
        // RealComponents keeps old versions
        QVERIFY(QFile::exists(installed));
        // the updated zip uses the info stored by the scan
        QCOMPARE(requestedPaths(), (QStringList{u"/api/mod/carryon"_s, u"/api/mods"_s, '/' + downloadPath()}));
    }

    // a rewrite here would mean an entry added on every scan
    void startupScanLeavesTheEntryOfADisabledModAlone() {
        RealComponents::Options options;
        options.mDisabledMods = {u"carryon@1.0.0"_s};
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        mServer->setFallback(info(modJson()));

        QVERIFY(mApp->start(options));

        QTRY_VERIFY(settled());
        QVERIFY(mApp->gameMngr().getDisabledMods().contains(u"carryon"_s));
        QCOMPARE(disabledModsOnDisk(mApp->gameDir()), QJsonArray{u"carryon@1.0.0"_s});
        QCOMPARE(requestedPaths(), (QStringList{u"/api/mod/carryon"_s, u"/api/mods"_s}));
    }

    void updateOfADisabledModKeepsItDisabled() {
        RealComponents::Options options;
        options.mDisabledMods = {u"carryon@1.0.0"_s};
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        const QByteArray zip = readFile(writeMod(mApp->stagingDir(), u"carryon"_s, u"1.1.0"_s));
        QVERIFY(!zip.isEmpty());
        const QJsonObject online = modJson({newerRelease()});
        mServer->enqueue(info(online));
        mServer->enqueue(download(zip));
        mServer->setFallback(info(online));
        QVERIFY(mApp->start(options));
        QTRY_VERIFY(settled());

        store().update(u"carryon"_s);

        QTRY_COMPARE(mod(u"carryon"_s)->getVersion(), ver(u"1.1.0"_s));
        QTRY_VERIFY(settled());
        QCOMPARE(disabledModsOnDisk(mApp->gameDir()), (QJsonArray{u"carryon@1.0.0"_s, u"carryon@1.1.0"_s}));
        QVERIFY(mApp->gameMngr().getDisabledMods().contains(u"carryon"_s));
    }

    static void nextSessionUsesTheStoredInfo_data() {
        QTest::addColumn<bool>("oldList");
        QTest::addColumn<QString>("released");
        QTest::addColumn<QStringList>("requested");

        QTest::newRow("fresh-list") << false << QString{RELEASED} << QStringList{};
        QTest::newRow("old-list") << true << QString{RELEASED} << QStringList{u"/api/mods"_s};
        QTest::newRow("old-list-newer-release")
            << true << u"2026-11-01 10:00:00"_s << QStringList{u"/api/mod/carryon"_s, u"/api/mods"_s};
    }

    void nextSessionUsesTheStoredInfo() {
        QFETCH(const bool, oldList);
        QFETCH(const QString, released);
        QFETCH(const QStringList, requested);
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        mServer->setFallback(info(modJson()));
        QVERIFY(mApp->start());
        QTRY_VERIFY(settled());

        if (oldList) {
            // an hour back, older than the loader's limit
            const QString connection = u"modloader-test"_s;
            {
                QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, connection);
                db.setDatabaseName(cacheDir() + "/modinfocache.db"_L1);
                QSqlQuery query{db};
                QVERIFY(db.open());
                QVERIFY(query.exec(u"UPDATE mod_list_stored SET stored_at = stored_at - 3600"_s));
                QCOMPARE(query.numRowsAffected(), 1);
            }
            QSqlDatabase::removeDatabase(connection);
        }
        // a new session over the same files and database, the loader goes before the store it points at
        mLoader.reset();
        mCache.reset();
        mCache = std::make_unique<vsmm::ModInfoCache>();
        const qsizetype firstSession = mServer->requestCount();
        mServer->route("/api/mods", modList(released));
        QVERIFY(mApp->restart());

        QTRY_VERIFY(settled());
        QVERIFY(hasOnlineInfo(u"carryon"_s));
        QCOMPARE(requestedPaths(firstSession), requested);
    }
};

QTEST_GUILESS_MAIN(ModLoaderIntegrationTest)
#include "ModLoaderIntegrationTest.moc"
