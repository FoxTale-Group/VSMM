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

#include <HttpClientMock.hpp>
#include <ImageTestUtils.hpp>
#include <ModImageProvider.hpp>
#include <ZipTestUtils.hpp>

#include <QBuffer>
#include <QColor>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QLoggingCategory>
#include <QQuickImageResponse>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <atomic>
#include <cstdint>
#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

namespace {
enum class UnusableIcon : std::uint8_t { Undecodable, Encrypted, MissingZip };
} // namespace

class ModImageProviderUnitTest : public QObject {
    Q_OBJECT

    static constexpr QLatin1StringView MOD_ID{"carryon"};
    static constexpr QLatin1StringView OTHER_MOD_ID{"petai"};
    static constexpr QLatin1StringView LOGO{"https://mods.vintagestory.at/files/asset/1/carryon.png"};
    static constexpr QLatin1StringView OTHER_LOGO{"https://mods.vintagestory.at/files/asset/2/petai.png"};
    // the only content type the provider ever asks for
    static constexpr QLatin1StringView IMAGE{"image/"};

    // only imageprovider logs, and no tracing: no test pins a debug line
    static void onlyImageProviderLogs() {
        QLoggingCategory::setFilterRules(u"*=false\nimageprovider=true\nimageprovider.debug=false"_s);
    }

    [[nodiscard]] static ResponsePtr request(vsmm::ModImageProvider &provider, const QString &id,
                                             const QSize requestedSize = {}) {
        return ResponsePtr{provider.requestImageResponse(id, requestedSize)};
    }

    // requestImageResponse only queues the fetch, so the loop has to turn before the mock sees it
    [[nodiscard]] static bool waitForRequests(const vsmm::HttpClientMock &http, const qsizetype count) {
        return QTest::qWaitFor([&http, count] { return http.callCount() >= count; });
    }

    // the queued fetch would already be there, so a short spin is enough to prove nothing else went out
    [[nodiscard]] static bool noRequestBeyond(const vsmm::HttpClientMock &http, const qsizetype count) {
        return !QTest::qWaitFor([&http, count] { return http.callCount() > count; }, 100);
    }

    // a zip read finishes on the decode pool, the answer reaches the response through the event loop
    [[nodiscard]] static Settled settledLater(QQuickImageResponse *response) {
        QSignalSpy finished{response, &QQuickImageResponse::finished};
        QTest::qVerify(finished.wait(), "response finished", "", __FILE__, __LINE__);
        return settled(response);
    }

    // an id without a zip fails to open it once, then falls back to the logo
    static void ignoreMissingZip(const int times = 1) {
        for (int i = 0; i < times; ++i) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression{u"^Failed to open zip archive : "_s});
        }
    }

    static void ignoreDecodeFailure(const QLatin1StringView modId) {
        QTest::ignoreMessage(QtWarningMsg,
                             qPrintable(u"Failed to decode icon for %1: invalid image data"_s.arg(modId)));
    }

    static void ignoreDownloadFailure(const QLatin1StringView modId, const QString &error) {
        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Failed to download icon for %1: %2"_s.arg(modId, error)));
    }

  private slots:
    void init() { onlyImageProviderLogs(); }

    void theLogoUrlSurvivesTheIdRoundTrip_data() {
        QTest::addColumn<QString>("logoUrl");
        QTest::newRow("plain") << QString{LOGO};
        // QUrlQuery splits on & and =, an unencoded nested url would lose everything after its first &
        QTest::newRow("nested-query") << u"https://mods.vintagestory.at/download?fileid=42&format=png"_s;
        // a literal + must not decay into a space, nor %2B into a +
        QTest::newRow("plus-and-escape") << u"https://mods.vintagestory.at/files/carry+on%2Bmore.png"_s;
        QTest::newRow("space") << u"https://mods.vintagestory.at/files/carry on.png"_s;
    }

    void theLogoUrlSurvivesTheIdRoundTrip() {
        ignoreMissingZip();
        QFETCH(QString, logoUrl);
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, logoUrl));

        QVERIFY(waitForRequests(http, 1));
        QCOMPARE(http.call(0).mUrl, QUrl{logoUrl});
        QCOMPARE(http.call(0).mContentType, QString{IMAGE});
        // the provider is the callbacks' lifetime guard
        QCOMPARE(http.call(0).mContext.data(), static_cast<QObject *>(&provider));
    }

    void aDownloadedIconResolvesThePendingResponse() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QSignalSpy downloaded{&provider, &vsmm::ModImageProvider::imageDownloaded};

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        // nothing is resolved while the download is in flight
        QVERIFY(settled(response.get()).mImage.isNull());
        QVERIFY(settled(response.get()).mError.isEmpty());
        QSignalSpy finished{response.get(), &QQuickImageResponse::finished};

        QVERIFY(http.succeed(0, pngBytes({8, 8}, Qt::red)));

        QTRY_COMPARE(finished.count(), 1);
        const Settled result = settled(response.get());
        QVERIFY(result.mError.isEmpty());
        QCOMPARE(result.mImage.size(), QSize(8, 8));
        QCOMPARE(result.mImage.pixelColor(0, 0), QColor{Qt::red});
        QCOMPARE(downloaded.count(), 1);
        // the whole id, a response only takes the answer for its own id
        QCOMPARE(downloaded.at(0).at(0).toString(), iconId(QString{MOD_ID}, QString{LOGO}));
    }

    void downloadProgressIsLoggedOnceTheTotalIsKnown() {
        ignoreMissingZip();
        // the progress line is a debug one, init() resets the rules for the next test
        QLoggingCategory::setFilterRules(u"*=false\nimageprovider=true"_s);
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));

        // an unknown total logs nothing
        QVERIFY(http.progress(0, 10, -1));
        QTest::ignoreMessage(QtDebugMsg, "Downloaded modicon for carryon (10 of 20)");
        QVERIFY(http.progress(0, 10, 20));

        // progress alone resolves nothing
        const Settled result = settled(response.get());
        QVERIFY(result.mImage.isNull());
        QVERIFY(result.mError.isEmpty());
    }

    void aValidRequestedSizeScalesTheDeliveredImageWithIgnoreAspectRatio() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}), QSize{8, 8});
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{response.get(), &QQuickImageResponse::finished};

        QVERIFY(http.succeed(0, pngBytes({16, 8})));

        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(settled(response.get()).mImage.size(), QSize(8, 8));
    }

    void aValidRequestedSizeScalesTheDeliveredImageWithKeepAspectRatio() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}), QSize{100, 100});
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{response.get(), &QQuickImageResponse::finished};

        QVERIFY(http.succeed(0, pngBytes({16, 8})));

        QTRY_COMPARE(finished.count(), 1);
        // 100 px and up keeps the aspect ratio
        QCOMPARE(settled(response.get()).mImage.size(), QSize(100, 50));
    }

    void aWideRequestedSizeKeepsTheAspectRatio() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}), QSize{100, 8});
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{response.get(), &QQuickImageResponse::finished};

        QVERIFY(http.succeed(0, pngBytes({16, 8})));

        QTRY_COMPARE(finished.count(), 1);
        // only the height is under 100, the ratio is kept
        QCOMPARE(settled(response.get()).mImage.size(), QSize(16, 8));
    }

    void aCachedIconIsServedWithoutASecondRequest() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr first = request(provider, iconId(QString{MOD_ID}, QString{LOGO}), QSize{8, 8});
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{first.get(), &QQuickImageResponse::finished};
        QVERIFY(http.succeed(0, pngBytes({16, 8})));
        QTRY_COMPARE(finished.count(), 1);

        const ResponsePtr second = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));

        // resolved before it was even handed back, and no new download
        const Settled result = settled(second.get());
        QVERIFY(result.mError.isEmpty());
        // the cache holds the source image, not the copy the first response scaled
        QCOMPARE(result.mImage.size(), QSize(16, 8));
        QVERIFY(noRequestBeyond(http, 1));
    }

    void twoRequestsForOneModShareASingleDownload() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr first = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        const ResponsePtr second = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        // scroll thrash collapses onto the one in-flight fetch
        QVERIFY(noRequestBeyond(http, 1));
        QSignalSpy firstFinished{first.get(), &QQuickImageResponse::finished};
        QSignalSpy secondFinished{second.get(), &QQuickImageResponse::finished};

        QVERIFY(http.succeed(0, pngBytes()));

        // the response that never issued a fetch has to be woken by the one that did
        QTRY_COMPARE(firstFinished.count(), 1);
        QTRY_COMPARE(secondFinished.count(), 1);
        QCOMPARE(settled(first.get()).mImage.size(), QSize(8, 8));
        QCOMPARE(settled(second.get()).mImage.size(), QSize(8, 8));
    }

    void eachModIsResolvedOnlyByItsOwnDownload() {
        ignoreMissingZip(2);
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr carryon = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        const ResponsePtr petai = request(provider, iconId(QString{OTHER_MOD_ID}, QString{OTHER_LOGO}));
        QVERIFY(waitForRequests(http, 2));
        QSignalSpy carryonFinished{carryon.get(), &QQuickImageResponse::finished};
        QSignalSpy petaiFinished{petai.get(), &QQuickImageResponse::finished};

        const qsizetype petaiCall = http.indexOf(QUrl{OTHER_LOGO});
        QVERIFY(petaiCall >= 0);
        QVERIFY(http.succeed(petaiCall, pngBytes({4, 4})));

        QTRY_COMPARE(petaiFinished.count(), 1);
        QCOMPARE(settled(petai.get()).mImage.size(), QSize(4, 4));
        // the other mod's icon must not leak into a row it was not asked for
        QCOMPARE(carryonFinished.count(), 0);
        QVERIFY(settled(carryon.get()).mImage.isNull());
    }

    void undecodableBytesMarkTheModAsHavingNoIcon() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr first = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{first.get(), &QQuickImageResponse::finished};

        ignoreDecodeFailure(MOD_ID);
        QVERIFY(http.succeed(0, QByteArray{"<html>404</html>"}));

        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(settled(first.get()).mError, u"Invalid image data"_s);

        // bytes that will not decode are a permanent answer: no retry on the next scroll into view
        const ResponsePtr second = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QCOMPARE(settled(second.get()).mError, u"No icon for mod %1"_s.arg(MOD_ID));
        QVERIFY(noRequestBeyond(http, 1));
    }

    void aDownloadFailureLeavesTheModRetryable() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr first = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{first.get(), &QQuickImageResponse::finished};

        ignoreDownloadFailure(MOD_ID, u"Timed out"_s);
        QVERIFY(http.fail(0, u"Timed out"_s));

        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(settled(first.get()).mError, u"Timed out"_s);

        // an outage is not permanent, the row asks again and this time it works
        const ResponsePtr second = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 2));
        QSignalSpy secondFinished{second.get(), &QQuickImageResponse::finished};
        QVERIFY(http.succeed(1, pngBytes()));

        QTRY_COMPARE(secondFinished.count(), 1);
        QCOMPARE(settled(second.get()).mImage.size(), QSize(8, 8));
    }

    void reloadingTheModsDropsEveryRememberedAnswer_data() {
        QTest::addColumn<bool>("undecodable");
        QTest::newRow("cached-icon") << false;
        QTest::newRow("no-icon") << true;
    }

    void reloadingTheModsDropsEveryRememberedAnswer() {
        ignoreMissingZip(2);
        QFETCH(bool, undecodable);
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr first = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{first.get(), &QQuickImageResponse::finished};
        if (undecodable) {
            ignoreDecodeFailure(MOD_ID);
            QVERIFY(http.succeed(0, QByteArray{"not an image"}));
        } else {
            QVERIFY(http.succeed(0, pngBytes()));
        }
        QTRY_COMPARE(finished.count(), 1);

        provider.onModsReloading();

        // a rescan starts from nothing, both the cache and the negative cache are gone
        const ResponsePtr second = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 2));
        QVERIFY(settled(second.get()).mImage.isNull());
        QVERIFY(settled(second.get()).mError.isEmpty());
    }

    void removingAModForgetsOnlyThatMod() {
        ignoreMissingZip(3);
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr carryon = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        const ResponsePtr petai = request(provider, iconId(QString{OTHER_MOD_ID}, QString{OTHER_LOGO}));
        QVERIFY(waitForRequests(http, 2));
        QSignalSpy downloaded{&provider, &vsmm::ModImageProvider::imageDownloaded};
        const qsizetype carryonCall = http.indexOf(QUrl{LOGO});
        const qsizetype petaiCall = http.indexOf(QUrl{OTHER_LOGO});
        QVERIFY(carryonCall >= 0);
        QVERIFY(petaiCall >= 0);
        QVERIFY(http.succeed(carryonCall, pngBytes()));
        QVERIFY(http.succeed(petaiCall, pngBytes({4, 4})));
        QTRY_COMPARE(downloaded.count(), 2);

        provider.onModRemoved(u"carryon");

        const ResponsePtr afterRemoval = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 3));
        QVERIFY(settled(afterRemoval.get()).mImage.isNull());
        // the mod that stayed installed keeps its icon
        const ResponsePtr untouched = request(provider, iconId(QString{OTHER_MOD_ID}, QString{OTHER_LOGO}));
        QCOMPARE(settled(untouched.get()).mImage.size(), QSize(4, 4));
        QVERIFY(noRequestBeyond(http, 3));
    }

    void removingAModClearsItsNoIconMark() {
        ignoreMissingZip(2);
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr first = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{first.get(), &QQuickImageResponse::finished};
        ignoreDecodeFailure(MOD_ID);
        QVERIFY(http.succeed(0, QByteArray{"not an image"}));
        QTRY_COMPARE(finished.count(), 1);

        provider.onModRemoved(u"carryon");

        // reinstalling the mod must not inherit the removed one's verdict
        const ResponsePtr second = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 2));
        QVERIFY(settled(second.get()).mError.isEmpty());
    }

    void aCancelledResponseIsNotResolvedTwice() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{response.get(), &QQuickImageResponse::finished};
        QSignalSpy downloaded{&provider, &vsmm::ModImageProvider::imageDownloaded};

        response->cancel();
        QCOMPARE(finished.count(), 1);
        QVERIFY(http.succeed(0, pngBytes()));
        QTRY_COMPARE(downloaded.count(), 1);

        // the late arrival must not reopen a response the view already gave up on
        QCOMPARE(finished.count(), 1);
        QCOMPARE(settled(response.get()).mError, u"Cancelled"_s);
        // it is still worth caching, the next row asking for it pays nothing
        const ResponsePtr second = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QCOMPARE(settled(second.get()).mImage.size(), QSize(8, 8));
        QVERIFY(noRequestBeyond(http, 1));
    }

    void anIconOutlivingItsResponseStillReachesTheCache() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QSignalSpy downloaded{&provider, &vsmm::ModImageProvider::imageDownloaded};

        ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        // the row scrolled out of view and the engine dropped the response mid-flight
        response.reset();

        QVERIFY(http.succeed(0, pngBytes()));
        QTRY_COMPARE(downloaded.count(), 1);

        const ResponsePtr second = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QCOMPARE(settled(second.get()).mImage.size(), QSize(8, 8));
        QVERIFY(noRequestBeyond(http, 1));
    }

    void anIconTooBigForTheCacheIsStillDelivered() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        // just past CACHE_SIZE: 4096 * 2049 * 4 bytes is 32 MiB plus one row
        const QByteArray oversized = pngBytes({4096, 2049});
        const ResponsePtr first = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{first.get(), &QQuickImageResponse::finished};

        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Failed to cache icon for %1"_s.arg(MOD_ID)));
        QVERIFY(http.succeed(0, oversized));

        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(settled(first.get()).mImage.size(), QSize(4096, 2049));

        // nothing was cached and nothing was marked missing, so the next row downloads it again
        const ResponsePtr second = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 2));
    }

    void aZipIconIsServedWithoutADownload() {
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString zip = dir.filePath(u"carryon.zip"_s);
        QVERIFY(writeArchive(zip, {{"modicon.png", pngBytes({16, 8}, Qt::red)}}));

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}, zip));

        const Settled result = settledLater(response.get());
        QVERIFY(result.mError.isEmpty());
        QCOMPARE(result.mImage.size(), QSize(16, 8));
        QCOMPARE(result.mImage.pixelColor(0, 0), QColor{Qt::red});
        QVERIFY(noRequestBeyond(http, 0));
    }

    void aCachedZipIconIsServedWithoutReadingTheZip() {
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString zip = dir.filePath(u"carryon.zip"_s);
        QVERIFY(writeArchive(zip, {{"modicon.png", pngBytes({16, 8}, Qt::red)}}));
        const QString id = iconId(QString{MOD_ID}, QString{LOGO}, zip);
        const ResponsePtr first = request(provider, id);
        QVERIFY(settledLater(first.get()).mError.isEmpty());

        QVERIFY(QFile::remove(zip));
        QTest::failOnWarning(QRegularExpression{u"zip archive"_s});
        const ResponsePtr second = request(provider, id);

        // resolved before it was even handed back
        QCOMPARE(settled(second.get()).mImage.size(), QSize(16, 8));
    }

    void aZipWithoutAnIconFallsBackToTheLogo() {
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString zip = dir.filePath(u"carryon.zip"_s);
        QVERIFY(writeArchive(zip, {{"modinfo.json", "{}"}}));
        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Modicon not found in zip archive %1"_s.arg(zip)));

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}, zip));
        QVERIFY(waitForRequests(http, 1));
        QCOMPARE(http.call(0).mUrl, QUrl{QString{LOGO}});
        QSignalSpy finished{response.get(), &QQuickImageResponse::finished};
        QVERIFY(http.succeed(0, pngBytes({8, 8}, Qt::blue)));

        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(settled(response.get()).mImage.pixelColor(0, 0), QColor{Qt::blue});
    }

    void anUnusableZipIconFallsBackToTheLogo_data() {
        QTest::addColumn<UnusableIcon>("unusable");
        QTest::newRow("undecodable") << UnusableIcon::Undecodable;
        QTest::newRow("encrypted") << UnusableIcon::Encrypted;
        QTest::newRow("missing-zip") << UnusableIcon::MissingZip;
    }

    void anUnusableZipIconFallsBackToTheLogo() {
        QFETCH(UnusableIcon, unusable);
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString zip = dir.filePath(u"carryon.zip"_s);
        if (unusable == UnusableIcon::Undecodable) {
            QVERIFY(writeArchive(zip, {{"modicon.png", "not an image"}}));
            ignoreDecodeFailure(MOD_ID);
        } else if (unusable == UnusableIcon::Encrypted) {
            if (!writeEncryptedArchive(zip, {"modicon.png", pngBytes()})) {
                QSKIP("libzip was built without encryption support");
            }
            QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Failed to retrieve modicon for %1"_s.arg(MOD_ID)));
        } else {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression{u"^Failed to open zip archive %1: "_s.arg(
                                                   QRegularExpression::escape(zip))});
        }

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}, zip));

        QVERIFY(waitForRequests(http, 1));
        QCOMPARE(http.call(0).mUrl, QUrl{QString{LOGO}});
    }

    void aModWithoutAnyIconAsksForNothing() {
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString zip = dir.filePath(u"carryon.zip"_s);
        QVERIFY(writeArchive(zip, {{"modinfo.json", "{}"}}));
        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Modicon not found in zip archive %1"_s.arg(zip)));
        // no online info yet, so no logo url either
        const QString id = iconId(QString{MOD_ID}, QString{}, zip);

        const ResponsePtr first = request(provider, id);
        QCOMPARE(settledLater(first.get()).mError, u"No icon url for mod %1"_s.arg(MOD_ID));

        // not a permanent answer, but the zip is remembered as having no icon
        QVERIFY(QFile::remove(zip));
        QTest::failOnWarning(QRegularExpression{u"zip archive"_s});
        const ResponsePtr second = request(provider, id);
        QCOMPARE(settledLater(second.get()).mError, u"No icon url for mod %1"_s.arg(MOD_ID));
        QVERIFY(noRequestBeyond(http, 0));
    }

    void aZipWithoutAnIconIsReadAgainOnceTheModIsForgotten_data() {
        QTest::addColumn<bool>("reload");
        QTest::newRow("removed") << false;
        QTest::newRow("reloaded") << true;
    }

    void aZipWithoutAnIconIsReadAgainOnceTheModIsForgotten() {
        QFETCH(bool, reload);
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString zip = dir.filePath(u"carryon.zip"_s);
        QVERIFY(writeArchive(zip, {{"modinfo.json", "{}"}}));
        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Modicon not found in zip archive %1"_s.arg(zip)));
        const QString id = iconId(QString{MOD_ID}, QString{}, zip);
        const ResponsePtr first = request(provider, id);
        QCOMPARE(settledLater(first.get()).mError, u"No icon url for mod %1"_s.arg(MOD_ID));

        if (reload) {
            provider.onModsReloading();
        } else {
            provider.onModRemoved(u"carryon");
        }
        QVERIFY(writeArchive(zip, {{"modicon.png", pngBytes({8, 8}, Qt::blue)}}));

        const ResponsePtr second = request(provider, id);
        QCOMPARE(settledLater(second.get()).mImage.pixelColor(0, 0), QColor{Qt::blue});
    }

    void removingAModDropsTheIconsOfAllItsVersions() {
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString older = dir.filePath(u"carryon-1.0.0.zip"_s);
        const QString newer = dir.filePath(u"carryon-1.1.0.zip"_s);
        const QString lib = dir.filePath(u"carryonlib.zip"_s);
        QVERIFY(writeArchive(older, {{"modicon.png", pngBytes({8, 8}, Qt::red)}}));
        QVERIFY(writeArchive(newer, {{"modicon.png", pngBytes({8, 8}, Qt::green)}}));
        QVERIFY(writeArchive(lib, {{"modicon.png", pngBytes({8, 8}, Qt::blue)}}));
        const QString olderId = iconId(u"carryon@1.0.0"_s, QString{LOGO}, older);
        const QString newerId = iconId(u"carryon@1.1.0"_s, QString{LOGO}, newer);
        // shares the carryon prefix, but is another mod
        const QString libId = iconId(u"carryonlib@1.0.0"_s, QString{OTHER_LOGO}, lib);

        // each version is an entry of its own
        const ResponsePtr olderFirst = request(provider, olderId);
        QCOMPARE(settledLater(olderFirst.get()).mImage.pixelColor(0, 0), QColor{Qt::red});
        const ResponsePtr newerFirst = request(provider, newerId);
        QCOMPARE(settledLater(newerFirst.get()).mImage.pixelColor(0, 0), QColor{Qt::green});
        const ResponsePtr libFirst = request(provider, libId);
        QCOMPARE(settledLater(libFirst.get()).mImage.pixelColor(0, 0), QColor{Qt::blue});

        provider.onModRemoved(u"carryon");
        QVERIFY(writeArchive(older, {{"modicon.png", pngBytes({8, 8}, Qt::yellow)}}));
        QVERIFY(writeArchive(newer, {{"modicon.png", pngBytes({8, 8}, Qt::yellow)}}));

        // both versions come from the rewritten zips, carryonlib still from the cache
        const ResponsePtr olderAgain = request(provider, olderId);
        QCOMPARE(settledLater(olderAgain.get()).mImage.pixelColor(0, 0), QColor{Qt::yellow});
        const ResponsePtr newerAgain = request(provider, newerId);
        QCOMPARE(settledLater(newerAgain.get()).mImage.pixelColor(0, 0), QColor{Qt::yellow});
        const ResponsePtr libAgain = request(provider, libId);
        QCOMPARE(settled(libAgain.get()).mImage.pixelColor(0, 0), QColor{Qt::blue});
        QVERIFY(noRequestBeyond(http, 0));
    }

    void aZipIconTooBigForTheCacheIsStillDelivered() {
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString zip = dir.filePath(u"carryon.zip"_s);
        // just past CACHE_SIZE: 4096 * 2049 * 4 bytes is 32 MiB plus one row
        QVERIFY(writeArchive(zip, {{"modicon.png", pngBytes({4096, 2049})}}));
        const QString id = iconId(QString{MOD_ID}, QString{LOGO}, zip);

        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Failed to cache icon for %1"_s.arg(MOD_ID)));
        const ResponsePtr first = request(provider, id);
        QCOMPARE(settledLater(first.get()).mImage.size(), QSize(4096, 2049));

        // nothing was cached, so the next row reads the zip again
        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Failed to cache icon for %1"_s.arg(MOD_ID)));
        const ResponsePtr second = request(provider, id);
        QCOMPARE(settledLater(second.get()).mImage.size(), QSize(4096, 2049));
        QVERIFY(noRequestBeyond(http, 0));
    }

    void theHttpClientIsSetOnce() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::HttpClientMock other;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        QTest::ignoreMessage(QtWarningMsg, "Http client already set");
        provider.setHttpClient(&other);

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));

        QVERIFY(waitForRequests(http, 1));
        // the second client is not wired up, it can never be asked for anything
        QCOMPARE(other.callCount(), 0);
    }

    void aReceiverMayCallBackIntoTheProviderWhileHandlingImageDownloaded() {
        ignoreMissingZip();
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        // direct connection, so the handler runs on the decode thread that is emitting
        std::atomic_bool reentered{false};
        connect(
            &provider, &vsmm::ModImageProvider::imageDownloaded, this,
            [&provider, &reentered](const QString &modId) {
                // mMutex is not recursive, this hangs if imageDownloaded is ever emitted under it
                provider.onModRemoved(modId);
                reentered = true;
            },
            Qt::DirectConnection);

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}));
        QVERIFY(waitForRequests(http, 1));
        QVERIFY(http.succeed(0, pngBytes()));

        QTRY_VERIFY(reentered);
    }
};

// not QTEST_GUILESS_MAIN: QQuickImageResponse::textureFactory, the only way to read back the image a
// response resolved with, goes through the scene graph, and that dereferences a render loop a bare
// QCoreApplication never creates. The offscreen platform is set here rather than in ctest's
// environment so the suite needs no display on any host.
int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app{argc, argv};
    ModImageProviderUnitTest testObject;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&testObject, argc, argv);
}

#include "ModImageProviderUnitTest.moc"
