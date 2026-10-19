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
#include <ModUpdateModel.hpp>
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
using Roles = vsmm::ModUpdateModel::Roles;
using State = vsmm::ModUpdateModel::State;

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

class ModUpdateModelIntegrationTest : public QObject {
    Q_OBJECT

    // only warnings, the info chatter is pinned by the unit suites
    static void onlyWarnings() {
        QLoggingCategory::setFilterRules(u"*=false\nmodloader=true\nmodloader.info=false\nmodloader.debug=false\n"
                                         u"modstore=true\nmodstore.info=false\nmodstore.debug=false\n"
                                         u"modupdatemodel=true\nmodupdatemodel.debug=false"_s);
    }

    std::unique_ptr<FakeHttpServer> mServer;
    std::unique_ptr<vsmm::HttpClient> mHttp;
    std::unique_ptr<RealComponents> mApp;
    std::unique_ptr<vsmm::ModLoader> mLoader;
    std::unique_ptr<vsmm::ModUpdateModel> mModel;
    QString mUpdateFileName;
    // every state row 0 went through, repeats collapsed
    QList<State> mStates;

    [[nodiscard]] vsmm::ModStore &store() const { return mApp->store(); }
    [[nodiscard]] bool idle() const { return !store().isWorkPending(); }
    [[nodiscard]] QVariant role(int row, int role) const { return mModel->data(mModel->index(row), role); }
    [[nodiscard]] State state(int row) const { return role(row, Roles::StateRole).value<State>(); }

    // same order as App
    void connectComponents() {
        mLoader = std::make_unique<vsmm::ModLoader>();
        mLoader->setApiUrl(mServer->url("api/mod/"_L1));
        mLoader->setHttpClient(mHttp.get());
        mLoader->setStore(&mApp->store());
        mLoader->setGameMngr(&mApp->gameMngr());
        mModel = std::make_unique<vsmm::ModUpdateModel>();
        mModel->setStore(&mApp->store());
        connect(mModel.get(), &QAbstractItemModel::dataChanged, this, [this] {
            if (mModel->rowCount(QModelIndex{}) > 0 && (mStates.isEmpty() || mStates.constLast() != state(0))) {
                mStates.append(state(0));
            }
        });
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
        mStates.clear();
        mUpdateFileName = u"vsmm-modupdatemodel-%1.zip"_s.arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        mServer = std::make_unique<FakeHttpServer>();
        QVERIFY(mServer->start());
        mHttp = std::make_unique<vsmm::HttpClient>();
        mApp = std::make_unique<RealComponents>();
        mApp->beforeValidate([this] { connectComponents(); });
    }

    void cleanup() {
        // waits for the worker pools, and goes before the store it points at
        mLoader.reset();
        mModel.reset();
        mApp.reset();
        mHttp.reset();
        mServer.reset();
        QFile::remove(tempPath(mUpdateFileName));
    }

    void updateAllShowsTheBatchFromQueuedToDone() {
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        const QByteArray zip = readFile(writeMod(mApp->stagingDir(), u"carryon"_s, u"1.1.0"_s));
        QVERIFY(!zip.isEmpty());
        const QJsonObject online = modJson({newerRelease()});
        mServer->enqueue(info(online));
        mServer->enqueue(download(zip));
        mServer->setFallback(info(online));
        QVERIFY(mApp->start());
        QTRY_VERIFY(idle());

        store().updateAll();

        // the whole batch is known before anything downloads
        QCOMPARE(mModel->count(), 1);
        QCOMPARE(state(0), State::Queued);
        QCOMPARE(role(0, Roles::IdRole).toString(), u"carryon"_s);
        QCOMPARE(role(0, Roles::FromVersionRole).toString(), u"1.0.0"_s);
        QCOMPARE(role(0, Roles::ToVersionRole).toString(), u"1.1.0"_s);

        QTRY_COMPARE(state(0), State::Done);
        QTRY_VERIFY(idle());
        QCOMPARE(mStates, (QList<State>{State::Downloading, State::Installing, State::Done}));
        // the store entry is the new version now, the row still shows where it came from
        QCOMPARE(store().find(u"carryon"_s)->getVersion(), ver(u"1.1.0"_s));
        QCOMPARE(role(0, Roles::FromVersionRole).toString(), u"1.0.0"_s);
        QCOMPARE(mModel->totalProgress(), 1.0);
        QVERIFY(!mModel->isActive());
    }

    void failedDownloadShowsAsFailed() {
        QVERIFY(!writeMod(mApp->modsDir(), u"carryon"_s, u"1.0.0"_s).isEmpty());
        const QJsonObject online = modJson({newerRelease()});
        mServer->enqueue(info(online));
        mServer->enqueue({.mStatus = 404});
        mServer->setFallback(info(online));
        QVERIFY(mApp->start());
        QTRY_VERIFY(idle());
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression{u"^Failed to retrieve update for carryon: "_s});
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression{u"^Update of carryon failed: "_s});

        store().update(u"carryon"_s);

        QTRY_COMPARE(state(0), State::Failed);
        QTRY_VERIFY(idle());
        QVERIFY(!role(0, Roles::ErrorRole).toString().isEmpty());
        QCOMPARE(mModel->failedCount(), 1);
        QCOMPARE(store().find(u"carryon"_s)->getVersion(), ver(u"1.0.0"_s));
    }
};

QTEST_GUILESS_MAIN(ModUpdateModelIntegrationTest)
#include "ModUpdateModelIntegrationTest.moc"
