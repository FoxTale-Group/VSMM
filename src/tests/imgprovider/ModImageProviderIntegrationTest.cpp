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
#include <ImageTestUtils.hpp>
#include <ModEntryTestUtils.hpp>
#include <ModImageProvider.hpp>
#include <RealComponents.hpp>

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

namespace {
// an 8x8 red png the way the mod api serves logos
[[nodiscard]] Response png(bool cacheable = false) {
    return {.mContentType = "image/png", .mBody = pngBytes(), .mCacheable = cacheable};
}
} // namespace

class ModImageProviderIntegrationTest : public QObject {
    Q_OBJECT

    // only imageprovider logs, the http client and the store are pinned by their own suites
    static void onlyImageProviderLogs() {
        QLoggingCategory::setFilterRules(u"*=false\nimageprovider=true\nimageprovider.debug=false"_s);
    }

    // where HttpClient keeps its QNetworkDiskCache
    [[nodiscard]] static QString cacheDir() { return QStandardPaths::writableLocation(QStandardPaths::CacheLocation); }

    std::unique_ptr<RealComponents> mApp;
    std::unique_ptr<FakeHttpServer> mServer;
    std::unique_ptr<vsmm::HttpClient> mHttp;
    std::unique_ptr<vsmm::ModImageProvider> mProvider;

    // a logo path per test and mod, so a disk cache entry never answers another test's request
    [[nodiscard]] QString iconOf(const QString &modId) const {
        const QByteArray path = QByteArray{QTest::currentTestFunction()} + '/' + modId.toLatin1() + ".png";
        return iconId(modId, mServer->url(QLatin1StringView{path}).toString());
    }

    [[nodiscard]] ResponsePtr request(const QString &modId) const {
        return ResponsePtr{mProvider->requestImageResponse(iconOf(modId), {})};
    }

    // a cache miss finishes later, the fetch is only queued when the response is handed back
    [[nodiscard]] static Settled settledLater(QQuickImageResponse *response) {
        QSignalSpy finished{response, &QQuickImageResponse::finished};
        QTest::qVerify(finished.wait(), "response finished", "", __FILE__, __LINE__);
        return settled(response);
    }

    // the queued fetch would already be out, so a short spin is enough to prove nothing else went out
    [[nodiscard]] bool noRequestBeyond(qsizetype count) const {
        return !QTest::qWaitFor([this, count] { return mServer->requestCount() > count; }, 100);
    }

    static void addMod(vsmm::ModStore &store, const QDir &dir, const QString &id) {
        store.add(localInfo(id, u"1.0.0"_s, writeStubZip(dir, u"%1.zip"_s.arg(id))), vsmm::ModStore::ModLoadType::Init);
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
        onlyImageProviderLogs();

        mApp = std::make_unique<RealComponents>();
        QVERIFY(mApp->start());
        mServer = std::make_unique<FakeHttpServer>();
        QVERIFY(mServer->start());
        mHttp = std::make_unique<vsmm::HttpClient>();
        mProvider = std::make_unique<vsmm::ModImageProvider>();
        mProvider->setHttpClient(mHttp.get());
        // connected the way App does it
        connect(&mApp->store(), &vsmm::ModStore::modsReloading, mProvider.get(),
                &vsmm::ModImageProvider::onModsReloading);
        connect(&mApp->store(), &vsmm::ModStore::modRemoved, mProvider.get(), &vsmm::ModImageProvider::onModRemoved);
    }

    void cleanup() {
        mProvider.reset();
        mHttp.reset();
        mServer.reset();
        mApp.reset();
    }

    void iconIsDownloadedDecodedAndServed() {
        mServer->setFallback(png());

        const ResponsePtr response = request(u"carryon"_s);
        const Settled result = settledLater(response.get());

        QVERIFY(result.mError.isEmpty());
        QCOMPARE(result.mImage.size(), QSize(8, 8));
        QCOMPARE(result.mImage.pixelColor(0, 0), QColor{Qt::red});
        QCOMPARE(mServer->requestCount(), 1);
        QVERIFY(mServer->requests().at(0).mTarget.endsWith("/carryon.png"));
    }

    void secondRequestIsServedFromMemory() {
        mServer->setFallback(png());
        const ResponsePtr first = request(u"carryon"_s);
        QVERIFY(settledLater(first.get()).mError.isEmpty());

        const ResponsePtr second = request(u"carryon"_s);

        // resolved before it was even handed back
        QCOMPARE(settled(second.get()).mImage.size(), QSize(8, 8));
        QVERIFY(noRequestBeyond(1));
    }

    // a rescan may bring new logos, so the store reload has to reach the provider
    void storeReloadRefetchesTheIcon() {
        mServer->setFallback(png());
        const ResponsePtr first = request(u"carryon"_s);
        QVERIFY(settledLater(first.get()).mError.isEmpty());

        mApp->store().reload();

        const ResponsePtr second = request(u"carryon"_s);
        QCOMPARE(settledLater(second.get()).mImage.size(), QSize(8, 8));
        QCOMPARE(mServer->requestCount(), 2);
    }

    void storeRemoveForgetsOnlyThatModsIcon() {
        mServer->setFallback(png());
        addMod(mApp->store(), mApp->modsDir(), u"carryon"_s);
        addMod(mApp->store(), mApp->modsDir(), u"petai"_s);
        const ResponsePtr carryOn = request(u"carryon"_s);
        const ResponsePtr petAi = request(u"petai"_s);
        QVERIFY(settledLater(carryOn.get()).mError.isEmpty());
        QVERIFY(settledLater(petAi.get()).mError.isEmpty());
        QCOMPARE(mServer->requestCount(), 2);

        mApp->store().remove(u"carryon"_s);

        const ResponsePtr petAiAgain = request(u"petai"_s);
        QCOMPARE(settled(petAiAgain.get()).mImage.size(), QSize(8, 8));
        QVERIFY(noRequestBeyond(2));
        const ResponsePtr carryOnAgain = request(u"carryon"_s);
        QVERIFY(settledLater(carryOnAgain.get()).mError.isEmpty());
        QCOMPARE(mServer->requestCount(), 3);
    }

    // a 404 is not remembered as a mod without an icon, the next request asks again
    void missingIconIsRetryable() {
        mServer->enqueue({.mStatus = 404});
        mServer->setFallback(png());
        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression{u"^Failed to download icon for carryon: .*HTTP Status: 404$"_s});

        const ResponsePtr first = request(u"carryon"_s);
        const Settled failed = settledLater(first.get());
        QVERIFY(failed.mImage.isNull());
        QVERIFY(!failed.mError.isEmpty());

        const ResponsePtr second = request(u"carryon"_s);
        QCOMPARE(settledLater(second.get()).mImage.size(), QSize(8, 8));
        QCOMPARE(mServer->requestCount(), 2);
    }

    // the api answering with a web page must not end up decoded as a logo
    void wrongContentTypeIsAnError() {
        mServer->setFallback({.mContentType = "text/html", .mBody = pngBytes()});
        QTest::ignoreMessage(QtWarningMsg,
                             "Failed to download icon for carryon: Received data with incorrect content type.");

        const ResponsePtr response = request(u"carryon"_s);
        const Settled result = settledLater(response.get());

        QVERIFY(result.mImage.isNull());
        QVERIFY(!result.mError.isEmpty());
    }

    // the memory cache is dropped on reload, a fresh disk cache entry still spares the network
    void freshDiskCacheEntrySurvivesAReload() {
        mServer->setFallback(png(true));
        const ResponsePtr first = request(u"carryon"_s);
        QVERIFY(settledLater(first.get()).mError.isEmpty());

        mApp->store().reload();

        const ResponsePtr second = request(u"carryon"_s);
        const Settled result = settledLater(second.get());
        QCOMPARE(result.mImage.size(), QSize(8, 8));
        QCOMPARE(result.mImage.pixelColor(0, 0), QColor{Qt::red});
        QCOMPARE(mServer->requestCount(), 1);
    }

    // the white icons came from a bad disk cache entry, a cut reply must never be the one served later
    void truncatedReplyIsRetriedAndOnlyTheFullIconIsCached() {
        Response cut = png(true);
        cut.mTruncateBody = true;
        mServer->enqueue(cut);
        mServer->setFallback(png(true));

        const ResponsePtr first = request(u"carryon"_s);
        QCOMPARE(settledLater(first.get()).mImage.size(), QSize(8, 8));
        QCOMPARE(mServer->requestCount(), 2);

        mApp->store().reload();

        const ResponsePtr second = request(u"carryon"_s);
        const Settled result = settledLater(second.get());
        QVERIFY(result.mError.isEmpty());
        QCOMPARE(result.mImage.pixelColor(7, 7), QColor{Qt::red});
        QCOMPARE(mServer->requestCount(), 2);
    }
};

// not QTEST_GUILESS_MAIN, textureFactory goes through the scene graph, see the unit suite
int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app{argc, argv};
    ModImageProviderIntegrationTest testObject;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&testObject, argc, argv);
}

#include "ModImageProviderIntegrationTest.moc"
