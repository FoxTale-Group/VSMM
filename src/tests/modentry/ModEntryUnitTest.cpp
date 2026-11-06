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

#include <ModEntry.hpp>
#include <ModEntryTestUtils.hpp>

#include <QJsonArray>
#include <QLoggingCategory>
#include <QTest>
#include <QTimeZone>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

namespace {

[[nodiscard]] vsmm::ModLocalInfo carryOnInfo(const QString &version = u"1.0.0"_s) {
    return {.mName = u"Carry On (local)"_s,
            .mId = u"carryon"_s,
            .mAuthors = {u"local author"_s},
            .mVersion = ver(version),
            .mFileInfo = QFileInfo{u"/mods/CarryOn-1.0.0.zip"_s}};
}

QJsonObject withKey(QJsonObject json, QLatin1StringView key, const QJsonValue &value) {
    json[key] = value;
    return json;
}

[[nodiscard]] QDateTime utc(QDate date, QTime time) { return QDateTime{date, time, QTimeZone::UTC}; }

// every field of a live /api/mod/carryon reply, cut down to two releases and one screenshot
[[nodiscard]] QJsonObject fullModJson() {
    const QJsonObject rc{{"releaseid"_L1, 61180},
                         {"mainfile"_L1, "https://moddbcdn.vintagestory.at/CarryOn_v2.0.0-rc.1.zip"_L1},
                         {"filename"_L1, "CarryOn_v2.0.0-rc.1.zip"_L1},
                         {"fileid"_L1, 130071},
                         {"downloads"_L1, 2394},
                         {"tags"_L1, QJsonArray{"1.22.0"_L1, "1.22.1"_L1}},
                         {"modidstr"_L1, "carryon"_L1},
                         {"modversion"_L1, "2.0.0-rc.1"_L1},
                         {"created"_L1, "2026-10-14 13:43:04"_L1},
                         {"changelog"_L1, "<p>first release candidate</p>"_L1}};
    const QJsonObject stable{{"releaseid"_L1, 60001},
                             {"mainfile"_L1, "https://moddbcdn.vintagestory.at/CarryOn_v1.14.3.zip"_L1},
                             {"filename"_L1, "CarryOn_v1.14.3.zip"_L1},
                             {"fileid"_L1, 129000},
                             {"downloads"_L1, 51000},
                             {"tags"_L1, QJsonArray{"1.21.0"_L1}},
                             {"modidstr"_L1, "carryon"_L1},
                             {"modversion"_L1, "1.14.3"_L1},
                             {"created"_L1, "2026-09-01 08:00:00"_L1},
                             {"changelog"_L1, "<p>fixes</p>"_L1}};
    const QJsonObject screenshot{
        {"fileid"_L1, 8045},
        {"mainfile"_L1, "https://moddbcdn.vintagestory.at/CarryOnLogo2.png"_L1},
        {"filename"_L1, "CarryOnLogo2.png"_L1},
        {"thumbnailfilename"_L1, "https://moddbcdn.vintagestory.at/CarryOnLogo2_55_60.png"_L1},
        {"created"_L1, "2022-10-30 03:46:26"_L1}};
    return {{"modid"_L1, 890},
            {"assetid"_L1, 4405},
            {"name"_L1, "Carry On"_L1},
            {"text"_L1, "<p>Pick up chests</p>"_L1},
            {"author"_L1, "NerdScurvy"_L1},
            {"urlalias"_L1, "carryon"_L1},
            {"logofile"_L1, "https://moddbcdn.vintagestory.at/CarryOnLogo.png"_L1},
            {"homepageurl"_L1, ""_L1},
            {"sourcecodeurl"_L1, "https://github.com/NerdScurvy/CarryOn"_L1},
            {"trailervideourl"_L1, ""_L1},
            {"issuetrackerurl"_L1, "https://github.com/NerdScurvy/CarryOn/issues"_L1},
            {"wikiurl"_L1, ""_L1},
            {"downloads"_L1, 1097804},
            {"follows"_L1, 8001},
            {"trendingpoints"_L1, -1},
            {"comments"_L1, 769},
            {"side"_L1, "both"_L1},
            {"type"_L1, "mod"_L1},
            {"created"_L1, "2022-10-22 01:30:37"_L1},
            {"lastmodified"_L1, "2026-10-21 17:55:19"_L1},
            {"lastreleased"_L1, "2026-10-14 14:48:50"_L1},
            {"tags"_L1, QJsonArray{"Storage"_L1, "QoL"_L1}},
            {"releases"_L1, QJsonArray{rc, stable}},
            {"screenshots"_L1, QJsonArray{screenshot}}};
}

} // namespace

class ModEntryUnitTest : public QObject {
    Q_OBJECT

    static constexpr auto GAME_VERSION = "1.22.5";

  private slots:
    void initTestCase() {
        // debug only, the pinned warnings still have to reach ignoreMessage
        QLoggingCategory::setFilterRules(u"*.debug=false"_s);
    }

    // local phase

    void localInfoStringifiesAsIdAtVersion() {
        const auto info = carryOnInfo(u"1.2.3-rc.1"_s);

        QCOMPARE(info.toString(), u"carryon@1.2.3-rc.1"_s);
        QCOMPARE(static_cast<QString>(info), u"carryon@1.2.3-rc.1"_s);
        QCOMPARE(info.versionString(), u"1.2.3-rc.1"_s);
    }

    void localInfoDefaultsToPlaceholders() {
        const vsmm::ModLocalInfo info;

        QVERIFY(info.mName.isEmpty());
        QVERIFY(info.mId.isEmpty());
        QCOMPARE(info.mDescription, u"None"_s);
        QCOMPARE(info.mSide, u"Unknown"_s);
        QCOMPARE(info.mAuthors, QStringList{u"Unknown"_s});
        QVERIFY(info.mDependencies.isEmpty());
        QVERIFY(info.mError.isEmpty());
    }

    void constructionExposesLocalInfo() {
        const vsmm::ModEntry entry{carryOnInfo()};

        QCOMPARE(entry.getId(), u"carryon"_s);
        QCOMPARE(entry.getName(), u"Carry On (local)"_s);
        QCOMPARE(entry.getAuthors(), QStringList{u"local author"_s});
        QCOMPARE(str(entry.getVersion()), u"1.0.0"_s);
        QCOMPARE(entry.getFileInfo().fileName(), u"CarryOn-1.0.0.zip"_s);
        QCOMPARE(entry.toString(), u"carryon@1.0.0"_s);
        QCOMPARE(static_cast<QString>(entry), u"carryon@1.0.0"_s);
    }

    void allLocalAuthorsAreKept() {
        auto info = carryOnInfo();
        info.mAuthors = {u"NerdScurvy"_s, u"Ghost"_s};
        const vsmm::ModEntry entry{std::move(info)};

        QCOMPARE(entry.getAuthors(), QStringList({u"NerdScurvy"_s, u"Ghost"_s}));
    }

    void onlineGettersAreEmptyBeforeInitOnlineInfo() {
        const vsmm::ModEntry entry{carryOnInfo()};

        QVERIFY(entry.getUrl().isEmpty());
        QVERIFY(entry.getTags().isEmpty());
        QVERIFY(entry.getSide().isEmpty());
        QVERIFY(entry.getLogoUrl().isEmpty());
        QVERIFY(!entry.hasUpdate());
        QVERIFY(!entry.getLatestVersion());
    }

    void flagsDefaultToFalseAndToggle() {
        vsmm::ModEntry entry{carryOnInfo()};
        QVERIFY(!entry.isMarkedForUpdate());
        QVERIFY(!entry.isFavorite());

        entry.setMarkedForUpdate(true);
        entry.setFavorite(true);
        QVERIFY(entry.isMarkedForUpdate());
        QVERIFY(entry.isFavorite());

        entry.setMarkedForUpdate(false);
        entry.setFavorite(false);
        QVERIFY(!entry.isMarkedForUpdate());
        QVERIFY(!entry.isFavorite());
    }

    // api reply parsing

    void fromJsonReadsEveryField() {
        vsmm::ModRelease rc;
        rc.mReleaseId = 61180;
        rc.mFileId = 130071;
        rc.mDownloads = 2394;
        rc.mModIdStr = u"carryon"_s;
        rc.mFileName = u"CarryOn_v2.0.0-rc.1.zip"_s;
        rc.mChangelog = u"<p>first release candidate</p>"_s;
        rc.mModVersion = ver(u"2.0.0-rc.1"_s);
        rc.mCreated = utc({2026, 10, 14}, {13, 43, 4});
        rc.mMainFile = QUrl{u"https://moddbcdn.vintagestory.at/CarryOn_v2.0.0-rc.1.zip"_s};
        rc.mGameVersions = {ver(u"1.22.0"_s), ver(u"1.22.1"_s)};

        vsmm::ModRelease stable;
        stable.mReleaseId = 60001;
        stable.mFileId = 129000;
        stable.mDownloads = 51000;
        stable.mModIdStr = u"carryon"_s;
        stable.mFileName = u"CarryOn_v1.14.3.zip"_s;
        stable.mChangelog = u"<p>fixes</p>"_s;
        stable.mModVersion = ver(u"1.14.3"_s);
        stable.mCreated = utc({2026, 9, 1}, {8, 0, 0});
        stable.mMainFile = QUrl{u"https://moddbcdn.vintagestory.at/CarryOn_v1.14.3.zip"_s};
        stable.mGameVersions = {ver(u"1.21.0"_s)};

        vsmm::ModScreenshot screenshot;
        screenshot.mFileId = 8045;
        screenshot.mMainFile = QUrl{u"https://moddbcdn.vintagestory.at/CarryOnLogo2.png"_s};
        screenshot.mThumbnail = QUrl{u"https://moddbcdn.vintagestory.at/CarryOnLogo2_55_60.png"_s};
        screenshot.mFileName = u"CarryOnLogo2.png"_s;
        screenshot.mCreated = utc({2022, 10, 30}, {3, 46, 26});

        vsmm::ModOnlineInfo expected;
        expected.mModId = 890;
        expected.mAssetId = 4405;
        expected.mDownloads = 1097804;
        expected.mFollows = 8001;
        expected.mComments = 769;
        expected.mTrendingPoints = -1;
        expected.mName = u"Carry On"_s;
        expected.mAuthor = u"NerdScurvy"_s;
        expected.mUrlAlias = u"carryon"_s;
        expected.mSide = u"both"_s;
        expected.mType = u"mod"_s;
        expected.mText = u"<p>Pick up chests</p>"_s;
        expected.mCreated = utc({2022, 10, 22}, {1, 30, 37});
        expected.mLastModified = utc({2026, 10, 21}, {17, 55, 19});
        expected.mLastReleased = utc({2026, 10, 14}, {14, 48, 50});
        expected.mLogo = QUrl{u"https://moddbcdn.vintagestory.at/CarryOnLogo.png"_s};
        expected.mSourceCode = QUrl{u"https://github.com/NerdScurvy/CarryOn"_s};
        expected.mIssueTracker = QUrl{u"https://github.com/NerdScurvy/CarryOn/issues"_s};
        expected.mTags = {u"Storage"_s, u"QoL"_s};
        // api order is kept, it is by release date and not by version
        expected.mReleases = {rc, stable};
        expected.mScreenshots = {screenshot};

        const auto info = vsmm::ModOnlineInfo::fromJson(fullModJson());

        // separate compares first, a failing ModOnlineInfo compare cannot print which field differs
        QVERIFY(info.mReleases == expected.mReleases);
        QVERIFY(info.mScreenshots == expected.mScreenshots);
        QVERIFY(info == expected);
    }

    void fromJsonTrimsTextFields() {
        QJsonObject json = modJson();
        json["name"_L1] = "\t Carry On \n"_L1;
        json["author"_L1] = " NerdScurvy "_L1;
        json["urlalias"_L1] = " carryon "_L1;

        const auto info = vsmm::ModOnlineInfo::fromJson(json);

        QCOMPARE(info.mName, u"Carry On"_s);
        QCOMPARE(info.mAuthor, u"NerdScurvy"_s);
        QCOMPARE(info.mUrlAlias, u"carryon"_s);
    }

    void wrongTypedFieldsParseAsEmpty() {
        const QJsonObject json{{"name"_L1, 42},          {"author"_L1, QJsonValue::Null}, {"urlalias"_L1, 7},
                               {"tags"_L1, "Storage"_L1}, {"releases"_L1, "1.1.0"_L1},    {"screenshots"_L1, 3}};

        const auto info = vsmm::ModOnlineInfo::fromJson(json);

        QVERIFY(info.mName.isEmpty());
        QVERIFY(info.mAuthor.isEmpty());
        QVERIFY(info.mUrlAlias.isEmpty());
        QVERIFY(info.mSide.isEmpty());
        QVERIFY(info.mTags.isEmpty());
        QVERIFY(info.mReleases.isEmpty());
        QVERIFY(info.mScreenshots.isEmpty());
        QVERIFY(!info.mLastReleased.isValid());
    }

    void nonStringTagsAreSkipped() {
        const auto info = vsmm::ModOnlineInfo::fromJson(
            withKey(modJson(), "tags"_L1, QJsonArray{"Storage"_L1, QJsonValue::Null, 4, "QoL"_L1}));

        QCOMPARE(info.mTags, QStringList({u"Storage"_s, u"QoL"_s}));
    }

    // the dst-gap row only bites in a zone with dst, ctest runs this suite with TZ=Europe/Warsaw
    void datesAreParsedAsUtc_data() {
        QTest::addColumn<QJsonValue>("text");
        QTest::addColumn<QDateTime>("expected");
        QTest::newRow("regular") << QJsonValue{"2026-10-20 23:26:45"_L1} << utc({2026, 10, 20}, {23, 26, 45});
        QTest::newRow("inside-dst-gap") << QJsonValue{"2026-03-29 02:30:00"_L1} << utc({2026, 3, 29}, {2, 30, 0});
        QTest::newRow("unparsable") << QJsonValue{"yesterday"_L1} << QDateTime{};
        QTest::newRow("missing") << QJsonValue{QJsonValue::Undefined} << QDateTime{};
    }

    void datesAreParsedAsUtc() {
        QFETCH(QJsonValue, text);
        QFETCH(QDateTime, expected);

        const auto info = vsmm::ModOnlineInfo::fromJson(withKey(modJson(), "lastreleased"_L1, text));

        QCOMPARE(info.mLastReleased.isValid(), expected.isValid());
        if (expected.isValid()) {
            QCOMPARE(info.mLastReleased, expected);
            QCOMPARE(info.mLastReleased.timeSpec(), Qt::UTC);
        }
    }

    // online phase

    void onlineInfoOverridesNameAuthorAndTypeAndTags() {
        vsmm::ModEntry entry{carryOnInfo()};

        entry.initOnlineInfo(onlineInfo(), ver(GAME_VERSION), false);

        QCOMPARE(entry.getName(), u"Carry On"_s);
        QCOMPARE(entry.getAuthors(), QStringList{u"NerdScurvy"_s});
        QCOMPARE(entry.getSide(), u"both"_s);
        QCOMPARE(entry.getTags(), QStringList({u"Storage"_s, u"QoL"_s}));
        // local version is what is installed, so online info must never touch it
        QCOMPARE(str(entry.getVersion()), u"1.0.0"_s);
        QCOMPARE(entry.getId(), u"carryon"_s);
    }

    void emptyOnlineNameAndAuthorKeepTheLocalOnes() {
        vsmm::ModEntry entry{carryOnInfo()};
        auto info = onlineInfo();
        info.mName.clear();
        info.mAuthor.clear();

        entry.initOnlineInfo(std::move(info), ver(GAME_VERSION), false);

        QCOMPARE(entry.getName(), u"Carry On (local)"_s);
        QCOMPARE(entry.getAuthors(), QStringList{u"local author"_s});
    }

    void urlAliasWinsOverAssetId() {
        vsmm::ModEntry entry{carryOnInfo()};

        entry.initOnlineInfo(onlineInfo(), ver(GAME_VERSION), false);

        QCOMPARE(entry.getUrl(), QUrl{u"https://mods.vintagestory.at/carryon"_s});
    }

    void assetIdIsUsedWhenAliasIsMissing_data() {
        QTest::addColumn<QJsonValue>("urlalias");
        QTest::newRow("null") << QJsonValue{QJsonValue::Null};
        QTest::newRow("missing") << QJsonValue{QJsonValue::Undefined};
        QTest::newRow("not-a-string") << QJsonValue{7};
        QTest::newRow("empty-string") << QJsonValue{""_L1};
        QTest::newRow("only-whitespace") << QJsonValue{"  "_L1};
    }

    void assetIdIsUsedWhenAliasIsMissing() {
        QFETCH(QJsonValue, urlalias);
        vsmm::ModEntry entry{carryOnInfo()};

        entry.initOnlineInfo(vsmm::ModOnlineInfo::fromJson(withKey(modJson(), "urlalias"_L1, urlalias)),
                             ver(GAME_VERSION), false);

        QCOMPARE(entry.getUrl(), QUrl{u"https://mods.vintagestory.at/show/mod/4405"_s});
    }

    // online phase, latest release & update detection

    // game is 1.22.5 and the release only advertises 1.22.0, patch numbers count as compatible
    void newerCompatibleReleaseIsAnUpdate() {
        vsmm::ModEntry entry{carryOnInfo()};

        entry.initOnlineInfo(onlineInfo({release(u"1.1.0"_s, {u"1.22.0"_s})}), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        const vsmm::ModRelease *latest = entry.getLatestVersion();
        QVERIFY(latest);
        QCOMPARE(str(latest->mModVersion), u"1.1.0"_s);
        QCOMPARE(latest->mFileName, u"CarryOn_v1.1.0.zip"_s);
        QCOMPARE(latest->mMainFile, QUrl{u"https://mods.vintagestory.at/download?fileid=1.1.0"_s});
    }

    // API orders releases by date, not by version, so keep the highest one that supports the game version
    void highestCompatibleReleaseWins_data() {
        QTest::addColumn<QString>("installedVersion");
        QTest::addColumn<QJsonArray>("releases");
        QTest::addColumn<QString>("expectedVersion");

        QTest::newRow("newest-first") << u"1.0.0"_s
                                      << QJsonArray{release(u"1.3.0"_s, {u"1.22.0"_s}),
                                                    release(u"1.2.0"_s, {u"1.22.0"_s}),
                                                    release(u"1.1.0"_s, {u"1.22.0"_s})}
                                      << u"1.3.0"_s;
        QTest::newRow("newer-behind-older")
            << u"1.0.0"_s << QJsonArray{release(u"1.0.0"_s, {u"1.22.0"_s}), release(u"2.0.0"_s, {u"1.22.0"_s})}
            << u"2.0.0"_s;
        // live /api/mod/carryon shape, a 1.x release published after the 2.0 prereleases sits first
        QTest::newRow("carryon-maintenance-release-first")
            << u"2.0.0-pre.1"_s
            << QJsonArray{release(u"1.14.3"_s, {u"1.22.0"_s}), release(u"2.0.0-pre.8"_s, {u"1.22.0"_s}),
                          release(u"2.0.0-pre.7"_s, {u"1.22.0"_s})}
            << u"2.0.0-pre.8"_s;
        // Terra Prety: a 6.x release published after the 7.x line, and 7.8.2 after 7.9.2
        QTest::newRow("terraprety-unsorted")
            << u"7.9.2"_s
            << QJsonArray{release(u"6.2.0"_s, {u"1.22.2"_s}), release(u"7.10.2"_s, {u"1.22.0"_s}),
                          release(u"7.8.2"_s, {u"1.22.0"_s}), release(u"7.10.1"_s, {u"1.22.0"_s})}
            << u"7.10.2"_s;
        // newest release dropped the installed game version, the previous one still has it
        QTest::newRow("newest-incompatible-falls-back")
            << u"1.0.0"_s << QJsonArray{release(u"2.0.0"_s, {u"1.23.0"_s}), release(u"1.5.0"_s, {u"1.22.0"_s})}
            << u"1.5.0"_s;
    }

    void highestCompatibleReleaseWins() {
        QFETCH(QString, installedVersion);
        QFETCH(QJsonArray, releases);
        QFETCH(QString, expectedVersion);
        vsmm::ModEntry entry{carryOnInfo(installedVersion)};

        entry.initOnlineInfo(onlineInfo(releases), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion()->mModVersion), expectedVersion);
    }

    void nothingNewerIsNotAnUpdate_data() {
        QTest::addColumn<QJsonArray>("releases");
        QTest::newRow("same-version") << QJsonArray{release(u"1.0.0"_s, {u"1.22.0"_s})};
        QTest::newRow("older-version") << QJsonArray{release(u"0.9.0"_s, {u"1.22.0"_s})};
        QTest::newRow("no-releases") << QJsonArray{};
    }

    void nothingNewerIsNotAnUpdate() {
        QFETCH(QJsonArray, releases);
        vsmm::ModEntry entry{carryOnInfo()};

        entry.initOnlineInfo(onlineInfo(releases), ver(GAME_VERSION), false);

        QVERIFY(!entry.hasUpdate());
        QVERIFY(!entry.getLatestVersion());
    }

    void incompatibleGameVersionIsNotAnUpdate_data() {
        QTest::addColumn<QStringList>("gameVersions");
        // an incompatible tag is silent, a tag that is not a version gets reported
        QTest::addColumn<QByteArray>("expectedWarning");
        QTest::newRow("older-minor") << QStringList{u"1.21.9"_s} << QByteArray{};
        QTest::newRow("newer-minor") << QStringList{u"1.23.0"_s} << QByteArray{};
        QTest::newRow("other-major") << QStringList{u"2.22.5"_s} << QByteArray{};
        QTest::newRow("unparsable") << QStringList{u"v1.22.5"_s}
                                    << QByteArray{"carryon: Cannot parse game version 'v1.22.5'"};
        QTest::newRow("empty-tag-list") << QStringList{} << QByteArray{};
    }

    void incompatibleGameVersionIsNotAnUpdate() {
        QFETCH(QStringList, gameVersions);
        QFETCH(QByteArray, expectedWarning);
        vsmm::ModEntry entry{carryOnInfo()};

        if (!expectedWarning.isEmpty()) {
            QTest::ignoreMessage(QtWarningMsg, expectedWarning.constData());
        }
        entry.initOnlineInfo(onlineInfo({release(u"1.1.0"_s, gameVersions)}), ver(GAME_VERSION), false);

        QVERIFY(!entry.hasUpdate());
    }

    void anyMatchingGameVersionTagIsEnough() {
        vsmm::ModEntry entry{carryOnInfo()};
        const QStringList gameVersions{u"1.20.4"_s, u"1.21.0"_s, u"1.22.1"_s};

        entry.initOnlineInfo(onlineInfo({release(u"1.1.0"_s, gameVersions)}), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
    }

    // a prerelease is eligible only when the installed version is one too, or the user opted in
    void prereleaseIsOnlyOfferedWhenEligible_data() {
        QTest::addColumn<QString>("installedVersion");
        QTest::addColumn<bool>("includePrerelease");
        QTest::addColumn<QString>("expectedVersion");

        QTest::newRow("stable-install-takes-the-stable-release") << u"1.0.0"_s << false << u"1.1.0"_s;
        QTest::newRow("prerelease-install-takes-the-prerelease") << u"2.0.0-pre.1"_s << false << u"2.0.0-pre.8"_s;
        QTest::newRow("opted-in-takes-the-prerelease") << u"1.0.0"_s << true << u"2.0.0-pre.8"_s;
    }

    void prereleaseIsOnlyOfferedWhenEligible() {
        QFETCH(QString, installedVersion);
        QFETCH(bool, includePrerelease);
        QFETCH(QString, expectedVersion);
        vsmm::ModEntry entry{carryOnInfo(installedVersion)};
        const QJsonArray releases{release(u"2.0.0-pre.8"_s, {u"1.22.0"_s}), release(u"1.1.0"_s, {u"1.22.0"_s})};

        entry.initOnlineInfo(onlineInfo(releases), ver(GAME_VERSION), includePrerelease);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion()->mModVersion), expectedVersion);
    }

    void releaseWithBadVersionIsDroppedButScanContinues_data() {
        QTest::addColumn<QJsonValue>("modversion");
        // dropping a release is only ok if it is also reported
        QTest::addColumn<QByteArray>("expectedWarning");
        QTest::newRow("missing") << QJsonValue{QJsonValue::Undefined}
                                 << QByteArray{"carryon: Cannot parse version ''"};
        QTest::newRow("not-a-string") << QJsonValue{9} << QByteArray{"carryon: Cannot parse version ''"};
        QTest::newRow("unparsable") << QJsonValue{"not-a-version"_L1}
                                    << QByteArray{"carryon: Cannot parse version 'not-a-version'"};
    }

    void releaseWithBadVersionIsDroppedButScanContinues() {
        QFETCH(QJsonValue, modversion);
        QFETCH(QByteArray, expectedWarning);
        vsmm::ModEntry entry{carryOnInfo()};
        const QJsonArray releases{withKey(release(u"9.9.9"_s, {u"1.22.0"_s}), "modversion"_L1, modversion),
                                  release(u"1.1.0"_s, {u"1.22.0"_s})};

        QTest::ignoreMessage(QtWarningMsg, expectedWarning.constData());
        auto info = onlineInfo(releases);
        QCOMPARE(info.mReleases.size(), 1);
        entry.initOnlineInfo(std::move(info), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion()->mModVersion), u"1.1.0"_s);
    }

    void secondInitOnlineInfoRefreshesTheEntry() {
        vsmm::ModEntry entry{carryOnInfo()};
        entry.initOnlineInfo(onlineInfo({release(u"1.1.0"_s, {u"1.22.0"_s})}), ver(GAME_VERSION), false);
        QVERIFY(entry.hasUpdate());

        entry.initOnlineInfo(onlineInfo({release(u"1.2.0"_s, {u"1.22.0"_s})}), ver(GAME_VERSION), false);
        QCOMPARE(str(entry.getLatestVersion()->mModVersion), u"1.2.0"_s);
        QCOMPARE(entry.getLatestVersion()->mFileName, u"CarryOn_v1.2.0.zip"_s);

        // an update that went away must not stick
        entry.initOnlineInfo(onlineInfo(), ver(GAME_VERSION), false);
        QVERIFY(!entry.hasUpdate());
        QVERIFY(!entry.getLatestVersion());
    }
};

QTEST_GUILESS_MAIN(ModEntryUnitTest)
#include "ModEntryUnitTest.moc"
