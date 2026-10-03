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
#include <ModLoader.hpp>
#include <RealComponents.hpp>
#include <ZipTestUtils.hpp>

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTest>
#include <QUuid>

#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

namespace {
[[nodiscard]] QByteArray modInfo(const QString &id, const QString &version) {
    const QJsonObject info{{"modid"_L1, id},
                           {"name"_L1, u"%1 (local)"_s.arg(id)},
                           {"authors"_L1, QJsonArray{"local author"_L1}},
                           {"version"_L1, version}};
    return QJsonDocument{info}.toJson(QJsonDocument::Compact);
}

// the status lives in the body, the transport status is 200 either way
[[nodiscard]] Response info(const QJsonObject &mod) {
    const QJsonObject body{{"statuscode"_L1, "200"_L1}, {"mod"_L1, mod}};
    return {.mBody = QJsonDocument{body}.toJson(QJsonDocument::Compact)};
}

[[nodiscard]] Response download(QByteArray zip) { return {.mContentType = "application/zip", .mBody = std::move(zip)}; }

[[nodiscard]] QString cacheDir() { return QStandardPaths::writableLocation(QStandardPaths::CacheLocation); }

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
    std::unique_ptr<RealComponents> mApp;
    std::unique_ptr<vsmm::ModLoader> mLoader;
    QString mUpdateFileName;

    [[nodiscard]] vsmm::ModStore &store() const { return mApp->store(); }
    [[nodiscard]] const vsmm::ModEntry *mod(const QString &id) const { return store().find(id); }

    // the store clears it half a second after the loader reports back
    [[nodiscard]] bool settled() const { return !store().isWorkPending(); }

    [[nodiscard]] bool hasOnlineInfo(const QString &id) const { return mod(id) && !mod(id)->getTags().isEmpty(); }

    // same order as App
    void connectLoader() {
        mLoader = std::make_unique<vsmm::ModLoader>();
        mLoader->setApiUrl(mServer->url("api/mod/"_L1));
        mLoader->setHttpClient(mHttp.get());
        mLoader->setStore(&mApp->store());
        mLoader->setGameMngr(&mApp->gameMngr());
    }

    [[nodiscard]] QStringList requestedPaths() const {
        QStringList paths;
        for (const auto &request : mServer->requests()) {
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
        mHttp = std::make_unique<vsmm::HttpClient>();
        mApp = std::make_unique<RealComponents>();
        mApp->beforeValidate([this] { connectLoader(); });
    }

    void cleanup() {
        // waits for the worker pools, and goes before the store it points at
        mLoader.reset();
        mApp.reset();
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
        QCOMPARE(requestedPaths(), QStringList{u"/api/mod/carryon"_s});
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
        QCOMPARE(requestedPaths(), (QStringList{u"/api/mod/betterruins"_s, u"/api/mod/carryon"_s}));
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
        QCOMPARE(mServer->requestCount(), 1);
    }

    void reloadRescansTheModsDir() {
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        mServer->setFallback(info(modJson()));
        QVERIFY(mApp->start());
        QTRY_VERIFY(settled());

        QVERIFY(!writeMod(mApp->modsDir(), u"betterruins"_s, u"1.0.0"_s).isEmpty());
        store().reload();
        QVERIFY(store().isWorkPending());

        QTRY_VERIFY(settled());
        QVERIFY(hasOnlineInfo(u"carryon"_s));
        QVERIFY(hasOnlineInfo(u"betterruins"_s));
        QCOMPARE(mServer->requestCount(), 3);
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
        QCOMPARE(requestedPaths(), (QStringList{u"/api/mod/carryon"_s, u"/api/mod/carryon"_s, '/' + downloadPath()}));
    }

    void updateOfAModWithoutOneClearsWorkPending() {
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        mServer->setFallback(info(modJson()));
        QVERIFY(mApp->start());
        QTRY_VERIFY(settled());
        QVERIFY(!mod(u"carryon"_s)->hasUpdate());

        store().update(u"carryon"_s);
        QVERIFY(store().isWorkPending());

        QTRY_VERIFY(settled());
        QCOMPARE(mServer->requestCount(), 1);
    }
};

QTEST_GUILESS_MAIN(ModLoaderIntegrationTest)
#include "ModLoaderIntegrationTest.moc"
