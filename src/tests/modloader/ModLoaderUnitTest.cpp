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

#include <GameMngrMock.hpp>
#include <HttpClientMock.hpp>
#include <ModEntryTestUtils.hpp>
#include <ModLoader.hpp>
#include <ModStoreMock.hpp>
#include <ZipTestUtils.hpp>

#include <QJsonDocument>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;
using LoadType = vsmm::IModStore::ModLoadType;

namespace {
constexpr auto MOD_ID = "carryon";
constexpr auto GAME_VERSION = "1.22.5";

enum class BrokenZip : std::uint8_t { NotAZip, Missing, NoModInfo, Encrypted };

[[nodiscard]] QJsonObject modInfo(const QString &id = QString::fromLatin1(MOD_ID)) {
    return {{"modid"_L1, id},
            {"name"_L1, "Carry On"_L1},
            {"authors"_L1, QJsonArray{"NerdScurvy"_L1, "Ghost"_L1}},
            {"version"_L1, "1.2.3"_L1}};
}

[[nodiscard]] QJsonObject modInfoWith(const QString &key, const QJsonValue &value) {
    QJsonObject info = modInfo();
    info[key] = value;
    return info;
}

[[nodiscard]] QJsonObject modInfoWithout(const QString &key) {
    QJsonObject info = modInfo();
    info.remove(key);
    return info;
}

[[nodiscard]] QByteArray toJson(const QJsonObject &json) { return QJsonDocument{json}.toJson(QJsonDocument::Compact); }

// statuscode is a string in the body, the transport status is always 200
[[nodiscard]] QByteArray apiResponse(const QJsonObject &mod = modJson()) {
    return toJson({{"statuscode"_L1, "200"_L1}, {"mod"_L1, mod}});
}

[[nodiscard]] QUrl apiUrl(const QString &id = QString::fromLatin1(MOD_ID)) {
    return QUrl{u"https://mods.vintagestory.at/api/mod/%1"_s.arg(id)};
}

[[nodiscard]] QString tempPath(const QString &fileName) {
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + fileName;
}

[[nodiscard]] QRegularExpression exactly(const QString &text) {
    return QRegularExpression{u"^%1$"_s.arg(QRegularExpression::escape(text))};
}

// the API hands out the file name of the newer release, the loader saves the download under it
[[nodiscard]] vsmm::ModEntry modWithUpdate(const QString &fileName) {
    QJsonObject newer = release(u"1.1.0"_s, {u"1.22.0"_s});
    newer["filename"_L1] = fileName;
    vsmm::ModEntry mod{localInfo(QString::fromLatin1(MOD_ID), u"1.0.0"_s, QFileInfo{})};
    mod.initOnlineInfo(modJson({newer}), ver(QString::fromLatin1(GAME_VERSION)), false);
    return mod;
}
} // namespace

class ModLoaderUnitTest : public QObject {
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> mDir;
    std::unique_ptr<vsmm::GameMngrMock> mGameMngr;
    std::unique_ptr<vsmm::HttpClientMock> mHttp;
    std::unique_ptr<vsmm::ModStoreMock> mStore;
    std::unique_ptr<vsmm::ModLoader> mLoader;
    std::unique_ptr<QSignalSpy> mFinished;
    QString mUpdateFileName;

    [[nodiscard]] QString modsPath(const QString &fileName) const {
        return QDir{mDir->path()}.absoluteFilePath(fileName);
    }

    // empty when libzip could not write the fixture
    [[nodiscard]] static QString writeZip(const QString &path, const QList<Entry> &entries) {
        return writeArchive(path, entries) ? QFileInfo{path}.absoluteFilePath() : QString{};
    }

    [[nodiscard]] QString writeMod(const QString &fileName, const QByteArray &modInfoJson) const {
        return writeZip(modsPath(fileName), {{"modinfo.json", modInfoJson}});
    }

    [[nodiscard]] QString writeMod(const QString &fileName = u"carryon.zip"_s) const {
        return writeMod(fileName, toJson(modInfo()));
    }

  private slots:
    // only this component logs, no tracing
    static void initTestCase() {
        QLoggingCategory::setFilterRules(u"*=false\nmodloader=true\nmodloader.debug=false"_s);
    }

    void init() {
        mDir = std::make_unique<QTemporaryDir>();
        QVERIFY(mDir->isValid());
        mUpdateFileName = u"vsmm-modloader-%1.zip"_s.arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

        mGameMngr = std::make_unique<vsmm::GameMngrMock>();
        mGameMngr->setModsDirs({QDir{mDir->path()}});
        mHttp = std::make_unique<vsmm::HttpClientMock>();
        mStore = std::make_unique<vsmm::ModStoreMock>();

        mLoader = std::make_unique<vsmm::ModLoader>();
        mLoader->setHttpClient(mHttp.get());
        mLoader->setGameMngr(mGameMngr.get());
        mLoader->setStore(mStore.get());
        mFinished = std::make_unique<QSignalSpy>(mLoader.get(), &vsmm::ModLoader::allModsReloaded);
    }

    void cleanup() {
        mFinished.reset();
        // waits for the worker pools
        mLoader.reset();
        mStore.reset();
        mHttp.reset();
        mGameMngr.reset();
        QFile::remove(tempPath(mUpdateFileName));
        mDir.reset();
    }

    void secondHttpClientIsIgnored() {
        vsmm::HttpClientMock other;
        QTest::ignoreMessage(QtWarningMsg, "HttpClient already set");
        mLoader->setHttpClient(&other);

        const QString zip = writeMod();
        QVERIFY(!zip.isEmpty());
        mLoader->load(QFileInfo{zip});

        QTRY_COMPARE(mHttp->callCount(), 1);
        QCOMPARE(other.callCount(), 0);
    }

    void secondGameMngrIsIgnored() {
        vsmm::GameMngrMock other;
        QTest::ignoreMessage(QtWarningMsg, "GameMngr already set");
        mLoader->setGameMngr(&other);

        // the other one has no dirs, scanning it would fail
        QTest::ignoreMessage(QtInfoMsg, "Scan started for 0 mods across 1 dirs");
        QVERIFY(mLoader->initModsList());
    }

    void secondStoreIsIgnored() {
        vsmm::ModStoreMock other;
        QTest::ignoreMessage(QtWarningMsg, "ModStore already set");
        mLoader->setStore(&other);

        other.reload();
        QCOMPARE(mFinished->count(), 0);

        QTest::ignoreMessage(QtInfoMsg, "Scan started for 0 mods across 1 dirs");
        mStore->reload();
        QCOMPARE(mFinished->count(), 1);
        QCOMPARE(mStore->reloadedCount(), 1);
        QCOMPARE(other.reloadedCount(), 0);
    }

    void storeReloadRescansTheModsDirs() {
        QVERIFY(!writeMod().isEmpty());

        QTest::ignoreMessage(QtInfoMsg, "Scan started for 1 mods across 1 dirs");
        mStore->reload();

        QTRY_COMPARE(mStore->addCalls().size(), 1);
        QCOMPARE(mStore->addCalls().at(0).mInfo.mId, QString::fromLatin1(MOD_ID));
    }

    void noModsDirsIsReported() {
        mGameMngr->setModsDirs({});

        QTest::ignoreMessage(QtCriticalMsg, "No mods dirs found");
        QVERIFY(!mLoader->initModsList());

        QCOMPARE(mFinished->count(), 1);
        QCOMPARE(mStore->reloadedCount(), 1);
    }

    void noModsDirsWaitsForPendingLoads() {
        QVERIFY(!writeMod().isEmpty());
        mLoader->load(QFileInfo{modsPath(u"carryon.zip"_s)});
        QTRY_COMPARE(mHttp->callCount(), 1);
        mGameMngr->setModsDirs({});

        QTest::ignoreMessage(QtCriticalMsg, "No mods dirs found");
        QVERIFY(!mLoader->initModsList());
        QCOMPARE(mFinished->count(), 0);

        QVERIFY(mHttp->succeed(0, apiResponse()));
        QTRY_COMPARE(mFinished->count(), 1);
    }

    void missingModsDirIsSkipped() {
        const QDir missing{modsPath(u"missing"_s)};
        mGameMngr->setModsDirs({missing, QDir{mDir->path()}});
        QVERIFY(!writeMod().isEmpty());

        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Mods path %1 does not exist"_s.arg(missing.path())));
        QTest::ignoreMessage(QtInfoMsg, "Scan started for 1 mods across 2 dirs");
        QVERIFY(mLoader->initModsList());

        QTRY_COMPARE(mStore->addCalls().size(), 1);
    }

    void emptyModsDirFinishesAtOnce() {
        QTest::ignoreMessage(QtInfoMsg, "Scan started for 0 mods across 1 dirs");
        QVERIFY(mLoader->initModsList());

        QCOMPARE(mFinished->count(), 1);
        QCOMPARE(mHttp->callCount(), 0);
    }

    void onlyZipFilesAreLoaded() {
        const QString zip = writeMod();
        QVERIFY(!zip.isEmpty());
        QVERIFY(writeStubZip(QDir{mDir->path()}, u"notes.txt"_s).exists());
        QVERIFY(QDir{mDir->path()}.mkdir(u"folder.zip"_s));

        QTest::ignoreMessage(QtInfoMsg, "Scan started for 1 mods across 1 dirs");
        QVERIFY(mLoader->initModsList());

        QTRY_COMPARE(mStore->addCalls().size(), 1);
        QCOMPARE(mStore->addCalls().at(0).mInfo.mFileInfo.absoluteFilePath(), zip);
    }

    void everyModsDirIsScanned() {
        QDir root{mDir->path()};
        QVERIFY(root.mkdir(u"first"_s));
        QVERIFY(root.mkdir(u"second"_s));
        mGameMngr->setModsDirs({QDir{modsPath(u"first"_s)}, QDir{modsPath(u"second"_s)}});
        QVERIFY(!writeMod(u"first/carryon.zip"_s).isEmpty());
        QVERIFY(!writeMod(u"second/betterruins.zip"_s, toJson(modInfo(u"betterruins"_s))).isEmpty());

        QTest::ignoreMessage(QtInfoMsg, "Scan started for 2 mods across 2 dirs");
        QVERIFY(mLoader->initModsList());

        QTRY_COMPARE(mStore->addCalls().size(), 2);
        QStringList ids{mStore->addCalls().at(0).mInfo.mId, mStore->addCalls().at(1).mInfo.mId};
        ids.sort();
        QCOMPARE(ids, (QStringList{u"betterruins"_s, u"carryon"_s}));
    }

    void finishesOnlyAfterTheLastMod() {
        QVERIFY(!writeMod().isEmpty());
        QVERIFY(!writeMod(u"betterruins.zip"_s, toJson(modInfo(u"betterruins"_s))).isEmpty());

        QTest::ignoreMessage(QtInfoMsg, "Scan started for 2 mods across 1 dirs");
        QVERIFY(mLoader->initModsList());
        QTRY_COMPARE(mHttp->callCount(), 2);
        QCOMPARE(mFinished->count(), 0);

        QVERIFY(mHttp->succeed(0, apiResponse()));
        QCOMPARE(mFinished->count(), 0);

        QVERIFY(mHttp->succeed(1, apiResponse()));
        QTRY_COMPARE(mFinished->count(), 1);
        QCOMPARE(mStore->onlineCalls().size(), 2);
    }

    void validZipIsAddedWithItsLocalInfo() {
        const QString zip = writeMod();
        QVERIFY(!zip.isEmpty());

        mLoader->load(QFileInfo{zip});

        QTRY_COMPARE(mStore->addCalls().size(), 1);
        const auto &[info, loadType] = mStore->addCalls().at(0);
        QCOMPARE(loadType, LoadType::Init);
        QCOMPARE(info.mId, QString::fromLatin1(MOD_ID));
        QCOMPARE(info.mName, u"Carry On"_s);
        QCOMPARE(info.mAuthor, u"NerdScurvy"_s);
        QCOMPARE(str(info.mVersion), u"1.2.3"_s);
        QCOMPARE(info.mFileInfo.absoluteFilePath(), zip);

        QCOMPARE(mHttp->callCount(), 1);
        QCOMPARE(mHttp->call(0).mUrl, apiUrl());
        QCOMPARE(mHttp->call(0).mContentType, u"application/json"_s);
        QCOMPARE(mHttp->call(0).mContext, mLoader.get());
        QCOMPARE(mFinished->count(), 0);
    }

    void infoIsRequestedFromTheConfiguredApiUrl() {
        mLoader->setApiUrl(QUrl{u"http://127.0.0.1:1/api/mod/"_s});
        QVERIFY(!writeMod().isEmpty());

        mLoader->load(QFileInfo{modsPath(u"carryon.zip"_s)});

        QTRY_COMPARE(mHttp->callCount(), 1);
        QCOMPARE(mHttp->call(0).mUrl, QUrl{u"http://127.0.0.1:1/api/mod/carryon"_s});
    }

    void loadFromGuiUsesTheGuiType() {
        const QString zip = writeMod();
        QVERIFY(!zip.isEmpty());

        mLoader->load(QUrl::fromLocalFile(zip));

        QTRY_COMPARE(mStore->addCalls().size(), 1);
        QCOMPARE(mStore->addCalls().at(0).mLoadType, LoadType::GUI);
        QCOMPARE(mStore->addCalls().at(0).mInfo.mFileInfo.absoluteFilePath(), zip);
    }

    static void unreadableZipIsReported_data() {
        QTest::addColumn<BrokenZip>("broken");
        QTest::addColumn<QString>("pattern");

        QTest::newRow("not-a-zip") << BrokenZip::NotAZip << u"Failed to open archive %1: .+"_s;
        QTest::newRow("missing-file") << BrokenZip::Missing << u"Failed to open archive %1: .+"_s;
        QTest::newRow("no-modinfo") << BrokenZip::NoModInfo
                                    << u"Failed to locate entry modinfo\\.json in archive %1: .+"_s;
        QTest::newRow("encrypted-modinfo") << BrokenZip::Encrypted << u"Failed to open entry 0 in archive %1: .+"_s;
    }

    void unreadableZipIsReported() {
        QFETCH(BrokenZip, broken);
        QFETCH(QString, pattern);

        const QString zip = modsPath(u"carryon.zip"_s);
        if (broken == BrokenZip::NotAZip) {
            QVERIFY(writeStubZip(QDir{mDir->path()}, u"carryon.zip"_s).exists());
        } else if (broken == BrokenZip::NoModInfo) {
            QVERIFY(!writeZip(zip, {{"readme.txt", "hello"}}).isEmpty());
        } else if (broken == BrokenZip::Encrypted && !writeEncryptedArchive(zip, {"modinfo.json", toJson(modInfo())})) {
            QSKIP("libzip was built without encryption support");
        }

        QTest::ignoreMessage(QtCriticalMsg,
                             QRegularExpression{u"^%1$"_s.arg(pattern.arg(QRegularExpression::escape(zip)))});
        mLoader->load(QFileInfo{zip});

        QTRY_COMPARE(mFinished->count(), 1);
        QVERIFY(mStore->addCalls().isEmpty());
        QCOMPARE(mHttp->callCount(), 0);
    }

    static void invalidModInfoIsReported_data() {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<bool>("withReason");

        QTest::newRow("invalid-json") << "{\"modid\": "_ba << true;
        QTest::newRow("array-root") << "[]"_ba << false;
        QTest::newRow("bad-version") << toJson(modInfoWith(u"version"_s, u"one.two"_s)) << false;
        QTest::newRow("non-string-version") << toJson(modInfoWith(u"version"_s, 1)) << false;
        QTest::newRow("missing-modid") << toJson(modInfoWithout(u"modid"_s)) << false;
        QTest::newRow("non-string-modid") << toJson(modInfoWith(u"modid"_s, 1)) << false;
        QTest::newRow("missing-name") << toJson(modInfoWithout(u"name"_s)) << false;
        QTest::newRow("non-string-name") << toJson(modInfoWith(u"name"_s, 1)) << false;
        QTest::newRow("missing-authors") << toJson(modInfoWithout(u"authors"_s)) << false;
        QTest::newRow("empty-authors") << toJson(modInfoWith(u"authors"_s, QJsonArray{})) << false;
        QTest::newRow("non-array-authors") << toJson(modInfoWith(u"authors"_s, u"NerdScurvy"_s)) << false;
        QTest::newRow("non-string-first-author") << toJson(modInfoWith(u"authors"_s, QJsonArray{1})) << false;
    }

    void invalidModInfoIsReported() {
        QFETCH(QByteArray, json);
        QFETCH(bool, withReason);

        const QString zip = writeMod(u"carryon.zip"_s, json);
        QVERIFY(!zip.isEmpty());

        const QString message = u"Failed to parse modinfo.json from zip file: %1"_s.arg(zip);
        if (withReason) {
            QTest::ignoreMessage(QtCriticalMsg,
                                 QRegularExpression{u"^%1 Reason: .+$"_s.arg(QRegularExpression::escape(message))});
        } else {
            QTest::ignoreMessage(QtCriticalMsg, qPrintable(message));
        }
        mLoader->load(QFileInfo{zip});

        QTRY_COMPARE(mFinished->count(), 1);
        QVERIFY(mStore->addCalls().isEmpty());
    }

    void modInfoKeysAreCaseInsensitive() {
        const QJsonObject json{{"ModID"_L1, "carryon"_L1},
                               {"NAME"_L1, "Carry On"_L1},
                               {"Authors"_L1, QJsonArray{"NerdScurvy"_L1}},
                               {"Version"_L1, "1.2.3"_L1}};
        QVERIFY(!writeMod(u"carryon.zip"_s, toJson(json)).isEmpty());

        mLoader->load(QFileInfo{modsPath(u"carryon.zip"_s)});

        QTRY_COMPARE(mStore->addCalls().size(), 1);
        const auto &info = mStore->addCalls().at(0).mInfo;
        QCOMPARE(info.mId, u"carryon"_s);
        QCOMPARE(info.mName, u"Carry On"_s);
        QCOMPARE(info.mAuthor, u"NerdScurvy"_s);
        QCOMPARE(str(info.mVersion), u"1.2.3"_s);
    }

    void onlineInfoIsHandedToTheStore() {
        QVERIFY(!writeMod().isEmpty());
        mLoader->load(QFileInfo{modsPath(u"carryon.zip"_s)});
        QTRY_COMPARE(mHttp->callCount(), 1);

        QVERIFY(mHttp->succeed(0, apiResponse()));

        QCOMPARE(mStore->onlineCalls().size(), 1);
        QCOMPARE(mStore->onlineCalls().at(0).mId, QString::fromLatin1(MOD_ID));
        QCOMPARE(mStore->onlineCalls().at(0).mOnlineInfo, modJson());
        QTRY_COMPARE(mFinished->count(), 1);
    }

    void onlineInfoFailureIsReported() {
        QVERIFY(!writeMod().isEmpty());
        mLoader->load(QFileInfo{modsPath(u"carryon.zip"_s)});
        QTRY_COMPARE(mHttp->callCount(), 1);

        QTest::ignoreMessage(QtWarningMsg, "Error retrieving info for carryon: timeout");
        QVERIFY(mHttp->fail(0, u"timeout"_s));

        QVERIFY(mStore->onlineCalls().isEmpty());
        QTRY_COMPARE(mFinished->count(), 1);
    }

    static void badOnlineInfoIsSkipped_data() {
        QTest::addColumn<QByteArray>("body");
        QTest::addColumn<QtMsgType>("type");
        QTest::addColumn<QRegularExpression>("message");

        QTest::newRow("invalid-json") << "{\"statuscode\": "_ba << QtWarningMsg
                                      << QRegularExpression{u"^Failed to parse online info for carryon: .+$"_s};
        QTest::newRow("not-an-object") << "[]"_ba << QtWarningMsg
                                       << exactly(u"Retrieved invalid response for carryon: not an object"_s);
        QTest::newRow("status-404") << toJson({{"statuscode"_L1, "404"_L1}}) << QtInfoMsg
                                    << exactly(u"Cannot retrieve online info for carryon: status 404"_s);
        QTest::newRow("numeric-statuscode") << toJson({{"statuscode"_L1, 200}, {"mod"_L1, modJson()}}) << QtInfoMsg
                                            << exactly(u"Cannot retrieve online info for carryon: status "_s);
        QTest::newRow("no-mod-object") << toJson({{"statuscode"_L1, "200"_L1}, {"mod"_L1, "carryon"_L1}})
                                       << QtWarningMsg
                                       << exactly(u"Retrieved invalid response for carryon: no mod object"_s);
    }

    void badOnlineInfoIsSkipped() {
        QFETCH(QByteArray, body);
        QFETCH(QtMsgType, type);
        QFETCH(QRegularExpression, message);

        QVERIFY(!writeMod().isEmpty());
        mLoader->load(QFileInfo{modsPath(u"carryon.zip"_s)});
        QTRY_COMPARE(mHttp->callCount(), 1);

        QTest::ignoreMessage(type, message);
        QVERIFY(mHttp->succeed(0, body));

        QVERIFY(mStore->onlineCalls().isEmpty());
        QTRY_COMPARE(mFinished->count(), 1);
    }

    void modWithoutUpdateFinishesWithoutDownload() {
        vsmm::ModEntry mod{localInfo(QString::fromLatin1(MOD_ID), u"1.0.0"_s, QFileInfo{})};
        mod.initOnlineInfo(modJson(), ver(QString::fromLatin1(GAME_VERSION)), false);
        QVERIFY(!mod.hasUpdate());

        mStore->requestUpdate(mod);

        QCOMPARE(mHttp->callCount(), 0);
        // the store would stay work pending otherwise
        QCOMPARE(mFinished->count(), 1);
        QCOMPARE(mStore->reloadedCount(), 1);
    }

    void modWithoutUpdateWaitsForPendingUpdates() {
        const vsmm::ModEntry pending = modWithUpdate(mUpdateFileName);
        mStore->requestUpdate(pending);
        QCOMPARE(mHttp->callCount(), 1);

        vsmm::ModEntry mod{localInfo(u"betterruins"_s, u"1.0.0"_s, QFileInfo{})};
        mod.initOnlineInfo(modJson(), ver(QString::fromLatin1(GAME_VERSION)), false);
        QVERIFY(!mod.hasUpdate());
        mStore->requestUpdate(mod);
        QCOMPARE(mFinished->count(), 0);

        QTest::ignoreMessage(QtWarningMsg, "Failed to retrieve update for carryon: timeout");
        QVERIFY(mHttp->fail(0, u"timeout"_s));
        QTRY_COMPARE(mFinished->count(), 1);
    }

    void updateIsDownloadedAsZip() {
        const vsmm::ModEntry mod = modWithUpdate(mUpdateFileName);
        QVERIFY(mod.hasUpdate());

        mStore->requestUpdate(mod);

        QCOMPARE(mHttp->callCount(), 1);
        QCOMPARE(mHttp->call(0).mUrl, mod.getLatestVersion().mUrl);
        QCOMPARE(mHttp->call(0).mContentType, u"application/zip"_s);
        QCOMPARE(mHttp->call(0).mContext, mLoader.get());
    }

    void updateDownloadFailureIsReported() {
        const vsmm::ModEntry mod = modWithUpdate(mUpdateFileName);
        mStore->requestUpdate(mod);
        QCOMPARE(mHttp->callCount(), 1);

        QTest::ignoreMessage(QtWarningMsg, "Failed to retrieve update for carryon: timeout");
        QVERIFY(mHttp->fail(0, u"timeout"_s));

        QTRY_COMPARE(mFinished->count(), 1);
        QVERIFY(mStore->addCalls().isEmpty());
    }

    void updateFailureOutlivesTheModEntry() {
        auto mod = std::make_unique<vsmm::ModEntry>(modWithUpdate(mUpdateFileName));
        mStore->requestUpdate(*mod);
        QCOMPARE(mHttp->callCount(), 1);

        mod.reset();
        // take the freed id buffer back at once, a view into it would read x's
        QStringList reused;
        for (int i = 0; i < 16; ++i) {
            reused.append(QString(qsizetype{7}, u'x'));
        }

        QTest::ignoreMessage(QtWarningMsg, "Failed to retrieve update for carryon: timeout");
        QVERIFY(mHttp->fail(0, u"timeout"_s));
        QTRY_COMPARE(mFinished->count(), 1);
    }

    void updateIsSavedAndLoaded() {
        const QString zip = writeMod();
        QVERIFY(!zip.isEmpty());
        QFile zipFile{zip};
        QVERIFY(zipFile.open(QIODevice::ReadOnly));
        const QByteArray download = zipFile.readAll();

        const vsmm::ModEntry mod = modWithUpdate(mUpdateFileName);
        mStore->requestUpdate(mod);
        QCOMPARE(mHttp->callCount(), 1);
        QVERIFY(mHttp->succeed(0, download));

        QTRY_COMPARE(mStore->addCalls().size(), 1);
        const QString saved = QFileInfo{tempPath(mUpdateFileName)}.absoluteFilePath();
        QCOMPARE(mStore->addCalls().at(0).mLoadType, LoadType::Update);
        QCOMPARE(mStore->addCalls().at(0).mInfo.mFileInfo.absoluteFilePath(), saved);
        QFile savedFile{saved};
        QVERIFY(savedFile.open(QIODevice::ReadOnly));
        QCOMPARE(savedFile.readAll(), download);

        QCOMPARE(mHttp->callCount(), 2);
        QCOMPARE(mHttp->call(1).mUrl, apiUrl());
        QCOMPARE(mFinished->count(), 0);
        QVERIFY(mHttp->succeed(1, apiResponse()));
        QTRY_COMPARE(mFinished->count(), 1);
    }

    void updateFileThatCannotBeCreatedIsReported() {
        // a sub dir that does not exist under the temp dir
        const QString fileName = QFileInfo{mUpdateFileName}.completeBaseName() + u"/CarryOn.zip"_s;
        const vsmm::ModEntry mod = modWithUpdate(fileName);
        mStore->requestUpdate(mod);
        QCOMPARE(mHttp->callCount(), 1);

        QTest::ignoreMessage(QtWarningMsg, qPrintable(u"Failed to create file for mod update %1"_s.arg(fileName)));
        QVERIFY(mHttp->succeed(0, "zip bytes"_ba));

        QTRY_COMPARE(mFinished->count(), 1);
        QVERIFY(mStore->addCalls().isEmpty());
    }
};

QTEST_GUILESS_MAIN(ModLoaderUnitTest)
#include "ModLoaderUnitTest.moc"
