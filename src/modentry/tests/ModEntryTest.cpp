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

#include <QJsonArray>
#include <QTest>

using namespace Qt::StringLiterals;

namespace {

[[nodiscard]] semver::version<> ver(const QString &version) {
    semver::version<> parsed;
    const auto result = semver::parse(version.toStdString(), parsed);
    // Not Q_ASSERT: that would abort the run and behave differently in release builds.
    QTest::qVerify(static_cast<bool>(result), "semver::parse(version)", qPrintable(version), __FILE__, __LINE__);
    return parsed;
}

[[nodiscard]] QString str(const semver::version<> &version) { return QString::fromStdString(version.to_string()); }

[[nodiscard]] vsmm::ModEntry::LocalInfo localInfo(const QString &version = u"1.0.0"_s) {
    return {.mName = u"Carry On (local)"_s,
            .mId = u"carryon"_s,
            .mAuthor = u"local author"_s,
            .mVersion = ver(version),
            .mFileInfo = QFileInfo{u"/mods/CarryOn-1.0.0.zip"_s}};
}

// One entry of the API's `releases` array. Game versions the release supports go into `tags`.
[[nodiscard]] QJsonObject release(const QString &modVersion, const QStringList &gameVersions) {
    QJsonArray tags;
    for (const auto &gameVersion : gameVersions) {
        tags.append(gameVersion);
    }
    return {{"modversion"_L1, modVersion},
            {"tags"_L1, tags},
            {"filename"_L1, u"CarryOn_v%1.zip"_s.arg(modVersion)},
            {"mainfile"_L1, u"https://mods.vintagestory.at/download?fileid=%1"_s.arg(modVersion)}};
}

// The `mod` object of an API response, trimmed down to what ModEntry reads.
// Tests that do not care about releases still get one, older than the installed version, so the
// outcome stays "no update" without tripping ModEntry's empty-releases warning.
[[nodiscard]] QJsonArray defaultReleases() { return {release(u"0.9.0"_s, {u"1.22.0"_s})}; }

[[nodiscard]] QJsonObject modJson(const QJsonArray &releases = defaultReleases()) {
    return {{"name"_L1, "Carry On"_L1}, {"author"_L1, "NerdScurvy"_L1},
            {"type"_L1, "mod"_L1},      {"urlalias"_L1, "carryon"_L1},
            {"assetid"_L1, 4405},       {"tags"_L1, QJsonArray{"Storage"_L1, "QoL"_L1}},
            {"releases"_L1, releases}};
}

QJsonObject withKey(QJsonObject json, QLatin1StringView key, const QJsonValue &value) {
    json[key] = value;
    return json;
}

QJsonObject withoutKey(QJsonObject json, QLatin1StringView key) {
    json.remove(key);
    return json;
}

} // namespace

class ModEntryTest : public QObject {
    Q_OBJECT

    static constexpr auto GAME_VERSION = "1.22.5";

  private slots:
    // --- Local phase -------------------------------------------------------

    void localInfoStringifiesAsIdAtVersion() {
        const auto info = localInfo(u"1.2.3-rc.1"_s);

        QCOMPARE(info.toString(), u"carryon@1.2.3-rc.1"_s);
        QCOMPARE(static_cast<QString>(info), u"carryon@1.2.3-rc.1"_s);
    }

    void constructionExposesLocalInfo() {
        const vsmm::ModEntry entry{localInfo()};

        QCOMPARE(entry.getId(), u"carryon"_s);
        QCOMPARE(entry.getName(), u"Carry On (local)"_s);
        QCOMPARE(entry.getAuthor(), u"local author"_s);
        QCOMPARE(str(entry.getVersion()), u"1.0.0"_s);
        QCOMPARE(entry.getFileInfo().fileName(), u"CarryOn-1.0.0.zip"_s);
        QCOMPARE(entry.toString(), u"carryon@1.0.0"_s);
        QCOMPARE(static_cast<QString>(entry), u"carryon@1.0.0"_s);
    }

    void onlineGettersAreEmptyBeforeInitOnlineInfo() {
        const vsmm::ModEntry entry{localInfo()};

        QVERIFY(entry.getUrl().isEmpty());
        QVERIFY(entry.getTags().isEmpty());
        QVERIFY(entry.getType().isEmpty());
        QVERIFY(!entry.hasUpdate());

        const auto &latest = entry.getLatestVersion();
        QVERIFY(!latest.mHasUpdate);
        QVERIFY(latest.mUrl.isEmpty());
        QVERIFY(latest.mFileName.isEmpty());
        QCOMPARE(str(latest.mVersion), str(semver::version<>{}));
    }

    void flagsDefaultToFalseAndToggle() {
        vsmm::ModEntry entry{localInfo()};
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

    // --- Online phase: scalar fields ---------------------------------------

    void onlineInfoOverridesNameAuthorAndTypeAndTags() {
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(modJson(), ver(GAME_VERSION), false);

        QCOMPARE(entry.getName(), u"Carry On"_s);
        QCOMPARE(entry.getAuthor(), u"NerdScurvy"_s);
        QCOMPARE(entry.getType(), u"mod"_s);
        QCOMPARE(entry.getTags(), QStringList({u"Storage"_s, u"QoL"_s}));
        // The local version is what is installed, so online info must never touch it.
        QCOMPARE(str(entry.getVersion()), u"1.0.0"_s);
        QCOMPARE(entry.getId(), u"carryon"_s);
    }

    void onlineNameIsTrimmed() {
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(withKey(modJson(), "name"_L1, "\t Carry On \n"_L1), ver(GAME_VERSION), false);

        QCOMPARE(entry.getName(), u"Carry On"_s);
    }

    void invalidNameFallsBackToLocalName_data() {
        QTest::addColumn<QJsonValue>("name");
        // Only a non-string name is reported; an empty or blank one is a valid string that simply
        // trims away, so it falls back silently.
        QTest::addColumn<bool>("warns");
        QTest::newRow("missing") << QJsonValue{QJsonValue::Undefined} << true;
        QTest::newRow("null") << QJsonValue{QJsonValue::Null} << true;
        QTest::newRow("number") << QJsonValue{42} << true;
        QTest::newRow("empty-string") << QJsonValue{""_L1} << false;
        QTest::newRow("only-whitespace") << QJsonValue{"   "_L1} << false;
    }

    void invalidNameFallsBackToLocalName() {
        QFETCH(QJsonValue, name);
        QFETCH(bool, warns);
        vsmm::ModEntry entry{localInfo()};

        if (warns) {
            QTest::ignoreMessage(QtWarningMsg, "\"Carry On (local): Invalid JSON format: name is not a string\"");
        }
        entry.initOnlineInfo(withKey(modJson(), "name"_L1, name), ver(GAME_VERSION), false);

        QCOMPARE(entry.getName(), u"Carry On (local)"_s);
    }

    void invalidAuthorKeepsLocalAuthor() {
        vsmm::ModEntry entry{localInfo()};

        QTest::ignoreMessage(QtWarningMsg, "\"Carry On: Invalid JSON format: author is not a string\"");
        entry.initOnlineInfo(withKey(modJson(), "author"_L1, 42), ver(GAME_VERSION), false);

        QCOMPARE(entry.getAuthor(), u"local author"_s);
    }

    void invalidTypeLeavesTypeEmpty() {
        vsmm::ModEntry entry{localInfo()};

        QTest::ignoreMessage(QtWarningMsg, "\"Carry On: Invalid JSON format: type is not a string\"");
        entry.initOnlineInfo(withoutKey(modJson(), "type"_L1), ver(GAME_VERSION), false);

        QVERIFY(entry.getType().isEmpty());
    }

    void nonArrayTagsLeaveTagsEmpty() {
        vsmm::ModEntry entry{localInfo()};

        QTest::ignoreMessage(QtWarningMsg, "\"Carry On: Invalid JSON format: tags is not a list\"");
        entry.initOnlineInfo(withKey(modJson(), "tags"_L1, "Storage"_L1), ver(GAME_VERSION), false);

        QVERIFY(entry.getTags().isEmpty());
    }

    void nullTagsAreSkipped() {
        vsmm::ModEntry entry{localInfo()};
        const QJsonArray tags{"Storage"_L1, QJsonValue::Null, "QoL"_L1};

        entry.initOnlineInfo(withKey(modJson(), "tags"_L1, tags), ver(GAME_VERSION), false);

        QCOMPARE(entry.getTags(), QStringList({u"Storage"_s, u"QoL"_s}));
    }

    // --- Online phase: mod url ---------------------------------------------

    void urlAliasWinsOverAssetId() {
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(modJson(), ver(GAME_VERSION), false);

        QCOMPARE(entry.getUrl(), QUrl{u"https://mods.vintagestory.at/carryon"_s});
    }

    void assetIdIsUsedWhenAliasIsMissing_data() {
        QTest::addColumn<QJsonValue>("urlalias");
        QTest::newRow("null") << QJsonValue{QJsonValue::Null};
        QTest::newRow("missing") << QJsonValue{QJsonValue::Undefined};
        QTest::newRow("not-a-string") << QJsonValue{7};
    }

    void assetIdIsUsedWhenAliasIsMissing() {
        QFETCH(QJsonValue, urlalias);
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(withKey(modJson(), "urlalias"_L1, urlalias), ver(GAME_VERSION), false);

        QCOMPARE(entry.getUrl(), QUrl{u"https://mods.vintagestory.at/show/mod/4405"_s});
    }

    void urlStaysEmptyWithoutAliasAndAssetId() {
        vsmm::ModEntry entry{localInfo()};

        QTest::ignoreMessage(QtWarningMsg, "\"Carry On: Invalid JSON format: assetid is not a number\"");
        entry.initOnlineInfo(withoutKey(withKey(modJson(), "urlalias"_L1, QJsonValue::Null), "assetid"_L1),
                             ver(GAME_VERSION), false);

        QVERIFY(entry.getUrl().isEmpty());
    }

    // --- Online phase: latest release / update detection --------------------

    // Game is 1.22.5 and the release only advertises 1.22.0: patch numbers are treated as compatible.
    void newerCompatibleReleaseIsAnUpdate() {
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(modJson({release(u"1.1.0"_s, {u"1.22.0"_s})}), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        const auto &latest = entry.getLatestVersion();
        QVERIFY(latest.mHasUpdate);
        QCOMPARE(str(latest.mVersion), u"1.1.0"_s);
        QCOMPARE(latest.mFileName, u"CarryOn_v1.1.0.zip"_s);
        QCOMPARE(latest.mUrl, QUrl{u"https://mods.vintagestory.at/download?fileid=1.1.0"_s});
    }

    // The API orders releases by publication date, not by version, so the scan has to look at every
    // release and keep the highest one that supports the installed game version.
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
        // Shape from the live /api/mod/carryon response: a 1.x maintenance release published after
        // the 2.0 prereleases sits first in the array.
        QTest::newRow("carryon-maintenance-release-first")
            << u"2.0.0-pre.1"_s
            << QJsonArray{release(u"1.14.3"_s, {u"1.22.0"_s}), release(u"2.0.0-pre.8"_s, {u"1.22.0"_s}),
                          release(u"2.0.0-pre.7"_s, {u"1.22.0"_s})}
            << u"2.0.0-pre.8"_s;
        // Terra Prety: a 6.x release published after the 7.x line, and 7.8.2 after 7.9.2.
        QTest::newRow("terraprety-unsorted")
            << u"7.9.2"_s
            << QJsonArray{release(u"6.2.0"_s, {u"1.22.2"_s}), release(u"7.10.2"_s, {u"1.22.0"_s}),
                          release(u"7.8.2"_s, {u"1.22.0"_s}), release(u"7.10.1"_s, {u"1.22.0"_s})}
            << u"7.10.2"_s;
        // The newest release dropped support for the installed game version; the previous one has it.
        QTest::newRow("newest-incompatible-falls-back")
            << u"1.0.0"_s << QJsonArray{release(u"2.0.0"_s, {u"1.23.0"_s}), release(u"1.5.0"_s, {u"1.22.0"_s})}
            << u"1.5.0"_s;
    }

    void highestCompatibleReleaseWins() {
        QFETCH(QString, installedVersion);
        QFETCH(QJsonArray, releases);
        QFETCH(QString, expectedVersion);
        vsmm::ModEntry entry{localInfo(installedVersion)};

        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), expectedVersion);
    }

    void installedVersionIsNotAnUpdate_data() {
        QTest::addColumn<QString>("releaseVersion");
        QTest::newRow("same-version") << u"1.0.0"_s;
        QTest::newRow("older-version") << u"0.9.0"_s;
    }

    void installedVersionIsNotAnUpdate() {
        QFETCH(QString, releaseVersion);
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(modJson({release(releaseVersion, {u"1.22.0"_s})}), ver(GAME_VERSION), false);

        QVERIFY(!entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), str(semver::version<>{}));
        QVERIFY(entry.getLatestVersion().mFileName.isEmpty());
    }

    void incompatibleGameVersionIsNotAnUpdate_data() {
        QTest::addColumn<QStringList>("gameVersions");
        // A tag that is merely incompatible is silent; one that is not a version is reported.
        QTest::addColumn<QByteArray>("expectedError");
        QTest::newRow("older-minor") << QStringList{u"1.21.9"_s} << QByteArray{};
        QTest::newRow("newer-minor") << QStringList{u"1.23.0"_s} << QByteArray{};
        QTest::newRow("other-major") << QStringList{u"2.22.5"_s} << QByteArray{};
        QTest::newRow("unparsable") << QStringList{u"v1.22.5"_s}
                                    << QByteArray{"\"Carry On: Cannot parse version 'v1.22.5'\""};
        QTest::newRow("empty-tag-list") << QStringList{} << QByteArray{};
    }

    void incompatibleGameVersionIsNotAnUpdate() {
        QFETCH(QStringList, gameVersions);
        QFETCH(QByteArray, expectedError);
        vsmm::ModEntry entry{localInfo()};

        if (!expectedError.isEmpty()) {
            QTest::ignoreMessage(QtCriticalMsg, expectedError.constData());
        }
        entry.initOnlineInfo(modJson({release(u"1.1.0"_s, gameVersions)}), ver(GAME_VERSION), false);

        QVERIFY(!entry.hasUpdate());
    }

    void anyMatchingGameVersionTagIsEnough() {
        vsmm::ModEntry entry{localInfo()};
        const QStringList gameVersions{u"1.20.4"_s, u"1.21.0"_s, u"1.22.1"_s};

        entry.initOnlineInfo(modJson({release(u"1.1.0"_s, gameVersions)}), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
    }

    // A prerelease is only eligible when the installed version is itself a prerelease, or the user
    // opted in. Every row offers the same two releases, so only those two inputs vary.
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
        vsmm::ModEntry entry{localInfo(installedVersion)};
        const QJsonArray releases{release(u"2.0.0-pre.8"_s, {u"1.22.0"_s}), release(u"1.1.0"_s, {u"1.22.0"_s})};

        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), includePrerelease);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), expectedVersion);
    }

    void malformedReleaseIsSkippedButScanContinues_data() {
        QTest::addColumn<QJsonObject>("badRelease");
        // Skipping a release is only acceptable if it is also reported.
        QTest::addColumn<QByteArray>("expectedError");
        QTest::newRow("modversion-missing") << withoutKey(release(u"9.9.9"_s, {u"1.22.0"_s}), "modversion"_L1)
                                            << QByteArray{"\"Carry On: Invalid JSON format: no modversion\""};
        QTest::newRow("modversion-not-a-string")
            << withKey(release(u"9.9.9"_s, {u"1.22.0"_s}), "modversion"_L1, QJsonValue{9})
            << QByteArray{"\"Carry On: Invalid JSON format: no modversion\""};
        QTest::newRow("modversion-unparsable")
            << withKey(release(u"9.9.9"_s, {u"1.22.0"_s}), "modversion"_L1, "not-a-version"_L1)
            << QByteArray{"\"Carry On: Cannot parse version 'not-a-version'\""};
        QTest::newRow("tags-not-an-array")
            << withKey(release(u"9.9.9"_s, {u"1.22.0"_s}), "tags"_L1, "1.22.0"_L1)
            << QByteArray{"\"Carry On: Invalid JSON format: release tags is not an array\""};
    }

    void malformedReleaseIsSkippedButScanContinues() {
        QFETCH(QJsonObject, badRelease);
        QFETCH(QByteArray, expectedError);
        vsmm::ModEntry entry{localInfo()};
        const QJsonArray releases{badRelease, release(u"1.1.0"_s, {u"1.22.0"_s})};

        QTest::ignoreMessage(QtCriticalMsg, expectedError.constData());
        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), u"1.1.0"_s);
    }

    void missingReleasesLeaveNoUpdate_data() {
        QTest::addColumn<QJsonValue>("releases");
        QTest::addColumn<QByteArray>("expectedError");
        const QByteArray notAnArray{"\"Carry On: Invalid JSON format: releases is not an array\""};
        QTest::newRow("missing") << QJsonValue{QJsonValue::Undefined} << notAnArray;
        QTest::newRow("null") << QJsonValue{QJsonValue::Null} << notAnArray;
        QTest::newRow("not-an-array") << QJsonValue{"1.1.0"_L1} << notAnArray;
        QTest::newRow("empty-array") << QJsonValue{QJsonArray{}}
                                     << QByteArray{"\"Carry On: Invalid JSON format: releases array is empty\""};
    }

    void missingReleasesLeaveNoUpdate() {
        QFETCH(QJsonValue, releases);
        QFETCH(QByteArray, expectedError);
        vsmm::ModEntry entry{localInfo()};

        QTest::ignoreMessage(QtWarningMsg, expectedError.constData());
        entry.initOnlineInfo(withKey(modJson(), "releases"_L1, releases), ver(GAME_VERSION), false);

        QVERIFY(!entry.hasUpdate());
        // The rest of the online info must still land.
        QCOMPARE(entry.getName(), u"Carry On"_s);
    }

    void secondInitOnlineInfoRefreshesTheEntry() {
        vsmm::ModEntry entry{localInfo()};
        entry.initOnlineInfo(modJson({release(u"1.1.0"_s, {u"1.22.0"_s})}), ver(GAME_VERSION), false);
        QVERIFY(entry.hasUpdate());

        entry.initOnlineInfo(modJson({release(u"1.2.0"_s, {u"1.22.0"_s})}), ver(GAME_VERSION), false);

        QCOMPARE(str(entry.getLatestVersion().mVersion), u"1.2.0"_s);
        QCOMPARE(entry.getLatestVersion().mFileName, u"CarryOn_v1.2.0.zip"_s);
    }
};

QTEST_GUILESS_MAIN(ModEntryTest)
#include "ModEntryTest.moc"
