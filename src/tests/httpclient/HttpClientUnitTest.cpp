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

#include "HttpClientTestUtils.hpp"

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTest>
#include <constants.hpp>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

class HttpClientUnitTest : public QObject {
    Q_OBJECT

    // the three content types the app actually asks for
    static constexpr QLatin1StringView JSON{"application/json"};
    static constexpr QLatin1StringView ZIP{"application/zip"};
    static constexpr QLatin1StringView IMAGE{"image/"};

    // mirrors HttpClient::MAX_RETRIES and BASE_BACKOFF, both private
    static constexpr int MAX_RETRIES{3};
    static constexpr int BASE_BACKOFF_MS{500};

    // only httpclient logs, and no tracing: no test pins a debug line
    static void onlyHttpClientLogs() {
        QLoggingCategory::setFilterRules(u"*=false\nhttpclient=true\nhttpclient.debug=false"_s);
    }

    static void ignoreStatusRetry(const int status) {
        QTest::ignoreMessage(QtWarningMsg,
                             qPrintable(u"Failed to retrieve data: HTTP status code %1. Retrying..."_s.arg(status)));
    }

    // the transport error text is the platform's, only the sentence around it is ours
    static void ignoreTransportRetry() {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression{u"^Failed to retrieve data: .+\\. Retrying\\.\\.\\.$"_s});
    }

  private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

    void init() { onlyHttpClientLogs(); }

    void successHandsTheBodyToTheSuccessCallback() {
        FakeHttpServer server;
        QVERIFY(server.start());
        server.enqueue({.mBody = R"({"statuscode":"200"})"});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        client.sendGet(server.url("success"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QCOMPARE(outcome.mCalls, 1);
        QVERIFY(outcome.mData);
        QCOMPARE(*outcome.mData, QByteArray{R"({"statuscode":"200"})"});
        QVERIFY(!outcome.mError);
        QCOMPARE(server.requestCount(), 1);
    }

    void requestIsAGetCarryingTheAppUserAgent() {
        FakeHttpServer server;
        QVERIFY(server.start());
        server.enqueue({});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        client.sendGet(server.url("user-agent"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QCOMPARE(server.requestCount(), 1);
        const Request &request = server.requests().constFirst();
        QCOMPARE(request.mMethod, QByteArray{"GET"});
        QCOMPARE(request.mTarget, QByteArray{"/user-agent"});
        QCOMPARE(request.header("user-agent"), u"%1/%2"_s.arg(APP_NAME, APP_VERSION).toUtf8());
    }

    void anEmptyBodyIsStillASuccess() {
        FakeHttpServer server;
        QVERIFY(server.start());
        server.enqueue({.mBody = {}});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        client.sendGet(server.url("empty"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QVERIFY(outcome.mData);
        QVERIFY(outcome.mData->isEmpty());
        QVERIFY(!outcome.mError);
    }

    void contentTypeIsMatchedByPrefix_data() {
        QTest::addColumn<QByteArray>("served");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<bool>("accepted");

        QTest::newRow("exact-json") << QByteArray{"application/json"} << QString{JSON} << true;
        QTest::newRow("json-with-charset") << QByteArray{"application/json; charset=utf-8"} << QString{JSON} << true;
        QTest::newRow("zip") << QByteArray{"application/zip"} << QString{ZIP} << true;
        // the icon pull asks for a bare "image/", which is the reason the check is a prefix at all
        QTest::newRow("png-for-image-prefix") << QByteArray{"image/png"} << QString{IMAGE} << true;
        QTest::newRow("webp-for-image-prefix") << QByteArray{"image/webp"} << QString{IMAGE} << true;
        // and the price of that prefix: a different type sharing the stem passes too
        QTest::newRow("jsonlines-for-json") << QByteArray{"application/jsonlines"} << QString{JSON} << true;

        QTest::newRow("html-for-json") << QByteArray{"text/html; charset=utf-8"} << QString{JSON} << false;
        QTest::newRow("json-for-zip") << QByteArray{"application/json"} << QString{ZIP} << false;
        QTest::newRow("json-for-image-prefix") << QByteArray{"application/json"} << QString{IMAGE} << false;
        QTest::newRow("no-content-type") << QByteArray{} << QString{JSON} << false;
    }

    void contentTypeIsMatchedByPrefix() {
        QFETCH(const QByteArray, served);
        QFETCH(const QString, expected);
        QFETCH(const bool, accepted);

        FakeHttpServer server;
        QVERIFY(server.start());
        server.enqueue({.mContentType = served, .mBody = "payload"});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        client.sendGet(server.url("content-type"_L1), &context, expected, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QCOMPARE(outcome.mCalls, 1);
        if (accepted) {
            QVERIFY(outcome.mData);
            QCOMPARE(*outcome.mData, QByteArray{"payload"});
        } else {
            QVERIFY(outcome.mError);
            QCOMPARE(*outcome.mError, u"Received data with incorrect content type."_s);
        }
        // a wrong content type is the server's answer, not a hiccup: it is never retried
        QCOMPARE(server.requestCount(), 1);
    }

    void permanentStatusIsReportedWithoutRetrying_data() {
        QTest::addColumn<int>("status");
        QTest::newRow("400") << 400;
        QTest::newRow("403") << 403;
        QTest::newRow("404") << 404;
        QTest::newRow("410") << 410;
        // the one 5xx that counts as permanent
        QTest::newRow("501") << 501;
    }

    void permanentStatusIsReportedWithoutRetrying() {
        QFETCH(const int, status);

        FakeHttpServer server;
        QVERIFY(server.start());
        server.setFallback({.mStatus = status});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        client.sendGet(server.url("permanent"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QCOMPARE(outcome.mCalls, 1);
        QVERIFY(!outcome.mData);
        QVERIFY(outcome.mError);
        // the reply's own text sits between these two, and it carries the port
        QVERIFY2(outcome.mError->startsWith(u"Error receiving response. Error: "_s), qPrintable(*outcome.mError));
        QVERIFY2(outcome.mError->endsWith(u"| HTTP Status: %1"_s.arg(status)), qPrintable(*outcome.mError));
        QCOMPARE(server.requestCount(), 1);
    }

    void retryableStatusIsRetriedThenSucceeds_data() {
        QTest::addColumn<int>("status");
        QTest::newRow("408") << 408;
        QTest::newRow("429") << 429;
        QTest::newRow("500") << 500;
        QTest::newRow("502") << 502;
        QTest::newRow("503") << 503;
        QTest::newRow("504") << 504;
    }

    void retryableStatusIsRetriedThenSucceeds() {
        QFETCH(const int, status);

        FakeHttpServer server;
        QVERIFY(server.start());
        // Retry-After: 0 keeps the wait out of the suite's wall clock, the backoff itself is pinned below
        server.enqueue({.mStatus = status, .mExtraHeaders = {{"Retry-After", "0"}}});
        server.enqueue({.mBody = "payload"});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        ignoreStatusRetry(status);
        client.sendGet(server.url("retryable"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QCOMPARE(outcome.mCalls, 1);
        QVERIFY(outcome.mData);
        QCOMPARE(*outcome.mData, QByteArray{"payload"});
        QCOMPARE(server.requestCount(), 2);
    }

    void retriesStopAtTheMaximum() {
        FakeHttpServer server;
        QVERIFY(server.start());
        server.setFallback({.mStatus = 503, .mExtraHeaders = {{"Retry-After", "0"}}});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
            ignoreStatusRetry(503);
        }
        client.sendGet(server.url("exhausted"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QCOMPARE(outcome.mCalls, 1);
        QVERIFY(outcome.mError);
        QVERIFY2(outcome.mError->endsWith(u"| HTTP Status: 503"_s), qPrintable(*outcome.mError));
        // the first attempt is not a retry, so MAX_RETRIES buys one more request than it says
        QCOMPARE(server.requestCount(), MAX_RETRIES + 1);
    }

    void retryAfterHeaderReplacesTheExponentialBackoff() {
        FakeHttpServer server;
        QVERIFY(server.start());
        for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
            server.enqueue({.mStatus = 429, .mExtraHeaders = {{"Retry-After", "0"}}});
            ignoreStatusRetry(429);
        }
        server.enqueue({.mBody = "payload"});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        QElapsedTimer elapsed;
        elapsed.start();
        client.sendGet(server.url("retry-after"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QVERIFY(outcome.mData);
        QCOMPARE(server.requestCount(), MAX_RETRIES + 1);
        // three doublings of BASE_BACKOFF would be 3500 ms, the header asked for none of it
        QVERIFY2(elapsed.elapsed() < BASE_BACKOFF_MS * 3, qPrintable(u"took %1 ms"_s.arg(elapsed.elapsed())));
    }

    void unusableRetryAfterFallsBackToTheExponentialBackoff_data() {
        QTest::addColumn<QByteArray>("retryAfter");
        QTest::newRow("absent") << QByteArray{};
        // the http-date form is legal HTTP and this client does not read it
        QTest::newRow("http-date") << QByteArray{"Wed, 21 Oct 2015 07:28:00 GMT"};
        QTest::newRow("not-a-number") << QByteArray{"soon"};
        QTest::newRow("negative") << QByteArray{"-1"};
    }

    void unusableRetryAfterFallsBackToTheExponentialBackoff() {
        QFETCH(const QByteArray, retryAfter);

        FakeHttpServer server;
        QVERIFY(server.start());
        Response first{.mStatus = 503};
        if (!retryAfter.isEmpty()) {
            first.mExtraHeaders = {{"Retry-After", retryAfter}};
        }
        server.enqueue(first);
        server.enqueue({.mBody = "payload"});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        ignoreStatusRetry(503);
        client.sendGet(server.url("bad-retry-after"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QElapsedTimer elapsed;
        elapsed.start();
        QTRY_VERIFY(outcome.settled());
        QVERIFY(outcome.mData);
        QCOMPARE(server.requestCount(), 2);
        // one retry, so the wait is BASE_BACKOFF; a coarse QTimer may fire up to 5% early
        QVERIFY2(elapsed.elapsed() >= BASE_BACKOFF_MS * 9 / 10, qPrintable(u"took %1 ms"_s.arg(elapsed.elapsed())));
    }

    void transportErrorIsRetriedThenSucceeds() {
        FakeHttpServer server;
        QVERIFY(server.start());
        server.enqueue({.mBody = "cut", .mTruncateBody = true});
        server.enqueue({.mBody = "payload"});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        ignoreTransportRetry();
        client.sendGet(server.url("dropped"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QCOMPARE(outcome.mCalls, 1);
        QVERIFY(outcome.mData);
        QCOMPARE(*outcome.mData, QByteArray{"payload"});
        QCOMPARE(server.requestCount(), 2);
    }

    void aRedirectIsFollowedWithoutTheCallerNoticing() {
        FakeHttpServer server;
        QVERIFY(server.start());
        server.enqueue({.mStatus = 302, .mContentType = {}, .mBody = {}, .mExtraHeaders = {{"Location", "/moved"}}});
        server.enqueue({.mBody = "payload"});
        Outcome outcome;
        vsmm::HttpClient client;
        QObject context;

        client.sendGet(server.url("redirect"_L1), &context, JSON, outcome.onSuccess(), outcome.onFailed());

        QTRY_VERIFY(outcome.settled());
        QVERIFY(outcome.mData);
        QCOMPARE(*outcome.mData, QByteArray{"payload"});
        QCOMPARE(server.requestCount(), 2);
        QCOMPARE(server.requests().at(1).mTarget, QByteArray{"/moved"});
    }

    void aFailureWithoutAFailedCallbackIsSilent() {
        FakeHttpServer server;
        QVERIFY(server.start());
        server.setFallback({.mStatus = 404});
        Outcome outcome;
        Outcome barrier;
        vsmm::HttpClient client;
        QObject context;

        client.sendGet(server.url("no-failed-callback"_L1), &context, JSON, outcome.onSuccess(), nullptr);

        // the omitted callback leaves nothing to wait on, so a second request that does settle is the barrier,
        // and it is only sent once the first one has been answered
        QTRY_COMPARE(server.requestCount(), 1);
        client.sendGet(server.url("barrier"_L1), &context, JSON, barrier.onSuccess(), barrier.onFailed());
        QTRY_VERIFY(barrier.settled());

        QVERIFY(barrier.mError);
        QCOMPARE(outcome.mCalls, 0);
        QCOMPARE(server.requestCount(), 2);
    }

    void aDestroyedContextDropsTheCallbacks() {
        FakeHttpServer server;
        QVERIFY(server.start());
        server.setFallback({.mBody = "payload"});
        Outcome outcome;
        Outcome barrier;
        vsmm::HttpClient client;
        QObject barrierContext;

        auto *context = new QObject;
        client.sendGet(server.url("orphaned"_L1), context, JSON, outcome.onSuccess(), outcome.onFailed());
        // the context is the callbacks' lifetime guard, the request itself is already on its way
        delete context;

        QTRY_COMPARE(server.requestCount(), 1);
        client.sendGet(server.url("barrier"_L1), &barrierContext, JSON, barrier.onSuccess(), barrier.onFailed());
        QTRY_VERIFY(barrier.settled());

        QVERIFY(barrier.mData);
        QCOMPARE(outcome.mCalls, 0);
    }
};

QTEST_GUILESS_MAIN(HttpClientUnitTest)
#include "HttpClientUnitTest.moc"
