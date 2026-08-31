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

#include <IHttpClient.hpp>

#include <QList>
#include <QPointer>

#include <utility>

namespace vsmm {
class HttpClientMock final : public IHttpClient {
  public:
    struct Call {
        QUrl mUrl;
        QPointer<QObject> mContext;
        QString mContentType;
        SuccessFn mOnSuccess;
        FailedFn mOnFailed;
    };

    ~HttpClientMock() override = default;

    void sendGet(const QUrl &url, QObject *context, const QString &contentType, SuccessFn successFn,
                 FailedFn failedFn) override {
        mCalls.append({url, context, contentType, std::move(successFn), std::move(failedFn)});
    }

    [[nodiscard]] qsizetype callCount() const { return mCalls.size(); }
    [[nodiscard]] const QList<Call> &calls() const { return mCalls; }
    [[nodiscard]] const Call &call(qsizetype index) const { return mCalls.at(index); }

    [[nodiscard]] qsizetype indexOf(const QUrl &url) const {
        for (qsizetype index = 0; index < mCalls.size(); ++index) {
            if (mCalls.at(index).mUrl == url) {
                return index;
            }
        }
        return -1;
    }

    [[nodiscard]] bool succeed(qsizetype index, QByteArray data) {
        const Call &call = mCalls.at(index);
        if (!call.mContext || !call.mOnSuccess) {
            return false;
        }
        call.mOnSuccess(std::move(data));
        return true;
    }

    [[nodiscard]] bool fail(qsizetype index, QString error) {
        const Call &call = mCalls.at(index);
        if (!call.mContext || !call.mOnFailed) {
            return false;
        }
        call.mOnFailed(std::move(error));
        return true;
    }

    void clear() { mCalls.clear(); }

  private:
    QList<Call> mCalls;
};
} // namespace vsmm
