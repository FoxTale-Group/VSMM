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

#include <HttpClient.hpp>

#include <QByteArray>
#include <QHostAddress>
#include <QList>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

#include <memory>
#include <optional>
#include <utility>

namespace vsmm::test {
using namespace Qt::StringLiterals;

// one canned reply, the defaults are the shape the mod api answers with
struct Response {
    int mStatus{200};
    QByteArray mContentType{"application/json"};
    QByteArray mBody{"{}"};
    QList<std::pair<QByteArray, QByteArray>> mExtraHeaders;
    bool mTruncateBody{false};
};

// what the server was asked for, header names are lowercased because HTTP does not case them
struct Request {
    QByteArray mMethod;
    QByteArray mTarget;
    QMap<QByteArray, QByteArray> mHeaders;

    [[nodiscard]] QByteArray header(const QByteArray &name) const { return mHeaders.value(name.toLower()); }
};

// QNetworkReply reports the phrase back through errorString, so the statuses used here carry a real one
[[nodiscard]] inline QByteArray reasonPhrase(const int status) {
    switch (status) {
    case 200:
        return "OK";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 400:
        return "Bad Request";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 408:
        return "Request Timeout";
    case 410:
        return "Gone";
    case 429:
        return "Too Many Requests";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavailable";
    case 504:
        return "Gateway Timeout";
    default:
        return "Unknown";
    }
}

class FakeHttpServer : public QTcpServer {
  public:
    explicit FakeHttpServer(QObject *parent = nullptr) : QTcpServer{parent} {
        connect(this, &QTcpServer::newConnection, this, [this] { acceptPending(); });
    }

    [[nodiscard]] bool start() { return listen(QHostAddress::LocalHost, 0); }

    // give every test its own path, so a disk cache entry can never answer another test's request
    [[nodiscard]] QUrl url(QLatin1StringView path) const {
        return QUrl{u"http://127.0.0.1:%1/%2"_s.arg(serverPort()).arg(path)};
    }

    // queued replies are handed out in order, the fallback answers everything after them
    void enqueue(Response response) { mQueued.append(std::move(response)); }
    void setFallback(Response response) { mFallback = std::move(response); }

    [[nodiscard]] qsizetype requestCount() const { return mRequests.size(); }
    [[nodiscard]] const QList<Request> &requests() const { return mRequests; }

  private:
    struct Conversation {
        QByteArray mBuffer;
        bool mAnswered{false};
    };

    void acceptPending() {
        while (QTcpSocket *socket = nextPendingConnection()) {
            auto conversation = std::make_shared<Conversation>();
            connect(socket, &QTcpSocket::readyRead, socket, [this, socket, conversation] {
                conversation->mBuffer.append(socket->readAll());
                const qsizetype headerEnd = conversation->mBuffer.indexOf("\r\n\r\n");
                // a GET carries no body, so the blank line is the whole request
                if (conversation->mAnswered || headerEnd < 0) {
                    return;
                }
                conversation->mAnswered = true;
                answer(socket, parse(conversation->mBuffer.first(headerEnd)));
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

    [[nodiscard]] static Request parse(const QByteArray &head) {
        const QByteArrayList lines = head.split('\n');
        Request request;
        const QByteArrayList requestLine = lines.value(0).trimmed().split(' ');
        request.mMethod = requestLine.value(0);
        request.mTarget = requestLine.value(1);

        for (qsizetype i = 1; i < lines.size(); ++i) {
            const QByteArray line = lines.at(i).trimmed();
            if (const qsizetype colon = line.indexOf(':'); colon > 0) {
                request.mHeaders.insert(line.first(colon).toLower(), line.sliced(colon + 1).trimmed());
            }
        }
        return request;
    }

    void answer(QTcpSocket *socket, Request request) {
        mRequests.append(std::move(request));
        const Response response = mQueued.isEmpty() ? mFallback : mQueued.takeFirst();

        const qsizetype promised = response.mBody.size() + (response.mTruncateBody ? TRUNCATED_BY : 0);
        QByteArray head = "HTTP/1.1 " + QByteArray::number(response.mStatus) + ' ' + reasonPhrase(response.mStatus) +
                          "\r\nContent-Length: " + QByteArray::number(promised) +
                          // nothing here is cacheable: QNetworkDiskCache is free to store a reply heuristically
                          // and the next request would then never reach this server
                          "\r\nCache-Control: no-store\r\nConnection: close\r\n";
        if (!response.mContentType.isEmpty()) {
            head += "Content-Type: " + response.mContentType + "\r\n";
        }
        for (const auto &[name, value] : response.mExtraHeaders) {
            head += name + ": " + value + "\r\n";
        }

        socket->write(head + "\r\n" + response.mBody);
        socket->flush();
        if (response.mTruncateBody) {
            socket->abort();
            return;
        }
        socket->disconnectFromHost();
    }

    // how much body a truncated reply promises and never sends
    static constexpr qsizetype TRUNCATED_BY{32};

    QList<Response> mQueued;
    Response mFallback;
    QList<Request> mRequests;
};

struct Outcome {
    std::optional<QByteArray> mData;
    std::optional<QString> mError;
    int mCalls{0};

    [[nodiscard]] bool settled() const { return mCalls > 0; }

    [[nodiscard]] HttpClient::SuccessFn onSuccess() {
        return [this](QByteArray data) {
            ++mCalls;
            mData = std::move(data);
        };
    }

    [[nodiscard]] HttpClient::FailedFn onFailed() {
        return [this](QString error) {
            ++mCalls;
            mError = std::move(error);
        };
    }
};
} // namespace vsmm::test
