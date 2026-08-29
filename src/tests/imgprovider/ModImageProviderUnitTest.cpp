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
#include <ModImageProvider.hpp>

#include <QBuffer>
#include <QColor>
#include <QGuiApplication>
#include <QImage>
#include <QLoggingCategory>
#include <QQuickImageResponse>
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

#include <atomic>
#include <memory>

using namespace Qt::StringLiterals;

namespace {

// a solid image encoded as png, built in memory so nothing binary is checked in
[[nodiscard]] QByteArray pngBytes(const QSize size = QSize{8, 8}, const QColor color = Qt::red) {
    QImage image{size, QImage::Format_ARGB32};
    image.fill(color);
    QByteArray bytes;
    QBuffer buffer{&bytes};
    // not Q_ASSERT, that would abort the run and behave differently in release builds
    QTest::qVerify(buffer.open(QIODevice::WriteOnly), "buffer.open(WriteOnly)", "", __FILE__, __LINE__);
    QTest::qVerify(image.save(&buffer, "PNG"), "image.save(PNG)", "", __FILE__, __LINE__);
    return bytes;
}

// the id ModListModel::IconRole builds, minus the image://modicon/ prefix the engine strips
[[nodiscard]] QString iconId(const QString &modId, const QString &logoUrl) {
    return u"%1?url=%2"_s.arg(modId, QString::fromLatin1(QUrl::toPercentEncoding(logoUrl)));
}

// what a response carries once it settled, textureFactory() hands its factory to the caller
struct Settled {
    QImage mImage;
    QString mError;
};

[[nodiscard]] Settled settled(const QQuickImageResponse *response) {
    const std::unique_ptr<QQuickTextureFactory> factory{response->textureFactory()};
    return {.mImage = factory ? factory->image() : QImage{}, .mError = response->errorString()};
}

// the provider hands ownership of the response to the engine, here the test owns it
using ResponsePtr = std::unique_ptr<QQuickImageResponse>;

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
        QCOMPARE(downloaded.at(0).at(0).toString(), QString{MOD_ID});
    }

    void aValidRequestedSizeScalesTheDeliveredImage() {
        vsmm::HttpClientMock http;
        vsmm::ModImageProvider provider;
        provider.setHttpClient(&http);

        const ResponsePtr response = request(provider, iconId(QString{MOD_ID}, QString{LOGO}), QSize{8, 8});
        QVERIFY(waitForRequests(http, 1));
        QSignalSpy finished{response.get(), &QQuickImageResponse::finished};

        QVERIFY(http.succeed(0, pngBytes({16, 8})));

        QTRY_COMPARE(finished.count(), 1);
        // KeepAspectRatio, so the 2:1 source fits the 8x8 box as 8x4
        QCOMPARE(settled(response.get()).mImage.size(), QSize(8, 4));
    }

    void aCachedIconIsServedWithoutASecondRequest() {
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
        QCOMPARE(settled(second.get()).mError, u"No icon for mod"_s);
        QVERIFY(noRequestBeyond(http, 1));
    }

    void aDownloadFailureLeavesTheModRetryable() {
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

    void theHttpClientIsSetOnce() {
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
