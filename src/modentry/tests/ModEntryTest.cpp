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
    Q_ASSERT_X(static_cast<bool>(result), "ver", qPrintable(version));
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
[[nodiscard]] QJsonObject modJson(const QJsonArray &releases = {}) {
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

    static void quietHandler(QtMsgType, const QMessageLogContext &, const QString &) {}
    QtMessageHandler mPreviousHandler{nullptr};

  private slots:
    void initTestCase() { mPreviousHandler = qInstallMessageHandler(quietHandler); }
    void cleanupTestCase() { qInstallMessageHandler(mPreviousHandler); }

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
        QTest::newRow("missing") << QJsonValue{QJsonValue::Undefined};
        QTest::newRow("null") << QJsonValue{QJsonValue::Null};
        QTest::newRow("number") << QJsonValue{42};
        QTest::newRow("empty string") << QJsonValue{""_L1};
        QTest::newRow("only whitespace") << QJsonValue{"   "_L1};
    }

    void invalidNameFallsBackToLocalName() {
        QFETCH(QJsonValue, name);
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(withKey(modJson(), "name"_L1, name), ver(GAME_VERSION), false);

        QCOMPARE(entry.getName(), u"Carry On (local)"_s);
    }

    void invalidAuthorKeepsLocalAuthor() {
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(withKey(modJson(), "author"_L1, 42), ver(GAME_VERSION), false);

        QCOMPARE(entry.getAuthor(), u"local author"_s);
    }

    void invalidTypeLeavesTypeEmpty() {
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(withoutKey(modJson(), "type"_L1), ver(GAME_VERSION), false);

        QVERIFY(entry.getType().isEmpty());
    }

    void nonArrayTagsLeaveTagsEmpty() {
        vsmm::ModEntry entry{localInfo()};

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
        QTest::newRow("not a string") << QJsonValue{7};
    }

    void assetIdIsUsedWhenAliasIsMissing() {
        QFETCH(QJsonValue, urlalias);
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(withKey(modJson(), "urlalias"_L1, urlalias), ver(GAME_VERSION), false);

        QCOMPARE(entry.getUrl(), QUrl{u"https://mods.vintagestory.at/show/mod/4405"_s});
    }

    void urlStaysEmptyWithoutAliasAndAssetId() {
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(withoutKey(withKey(modJson(), "urlalias"_L1, QJsonValue::Null), "assetid"_L1),
                             ver(GAME_VERSION), false);

        QVERIFY(entry.getUrl().isEmpty());
    }

    // --- Online phase: latest release / update detection --------------------

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

    void newestReleaseWinsWhenSeveralAreCompatible() {
        vsmm::ModEntry entry{localInfo()};
        // The API returns releases newest first.
        const QJsonArray releases{release(u"1.3.0"_s, {u"1.22.0"_s}), release(u"1.2.0"_s, {u"1.22.0"_s}),
                                  release(u"1.1.0"_s, {u"1.22.0"_s})};

        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), false);

        QCOMPARE(str(entry.getLatestVersion().mVersion), u"1.3.0"_s);
    }

    void installedVersionIsNotAnUpdate_data() {
        QTest::addColumn<QString>("releaseVersion");
        QTest::newRow("same version") << u"1.0.0"_s;
        QTest::newRow("older version") << u"0.9.0"_s;
    }

    void installedVersionIsNotAnUpdate() {
        QFETCH(QString, releaseVersion);
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(modJson({release(releaseVersion, {u"1.22.0"_s})}), ver(GAME_VERSION), false);

        QVERIFY(!entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), str(semver::version<>{}));
        QVERIFY(entry.getLatestVersion().mFileName.isEmpty());
    }

    void gameVersionPatchDifferenceIsCompatible() {
        vsmm::ModEntry entry{localInfo()};
        // Game is 1.22.5, the release only advertises 1.22.0 - patch numbers are treated as compatible.
        entry.initOnlineInfo(modJson({release(u"1.1.0"_s, {u"1.22.0"_s})}), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
    }

    void incompatibleGameVersionIsNotAnUpdate_data() {
        QTest::addColumn<QStringList>("gameVersions");
        QTest::newRow("older minor") << QStringList{u"1.21.9"_s};
        QTest::newRow("newer minor") << QStringList{u"1.23.0"_s};
        QTest::newRow("other major") << QStringList{u"2.22.5"_s};
        QTest::newRow("unparsable") << QStringList{u"v1.22.5"_s};
        QTest::newRow("empty tag list") << QStringList{};
    }

    void incompatibleGameVersionIsNotAnUpdate() {
        QFETCH(QStringList, gameVersions);
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(modJson({release(u"1.1.0"_s, gameVersions)}), ver(GAME_VERSION), false);

        QVERIFY(!entry.hasUpdate());
    }

    void anyMatchingGameVersionTagIsEnough() {
        vsmm::ModEntry entry{localInfo()};
        const QStringList gameVersions{u"1.20.4"_s, u"1.21.0"_s, u"1.22.1"_s};

        entry.initOnlineInfo(modJson({release(u"1.1.0"_s, gameVersions)}), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
    }

    void prereleaseIsSkippedForAStableInstall() {
        vsmm::ModEntry entry{localInfo()};
        const QJsonArray releases{release(u"2.0.0-pre.8"_s, {u"1.22.0"_s}), release(u"1.1.0"_s, {u"1.22.0"_s})};

        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), u"1.1.0"_s);
    }

    void prereleaseIsOfferedWhenInstalledVersionIsAPrerelease() {
        vsmm::ModEntry entry{localInfo(u"2.0.0-pre.1"_s)};

        entry.initOnlineInfo(modJson({release(u"2.0.0-pre.8"_s, {u"1.22.0"_s})}), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), u"2.0.0-pre.8"_s);
    }

    void prereleaseIsOfferedWhenEnabledInConfig() {
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(modJson({release(u"2.0.0-pre.8"_s, {u"1.22.0"_s})}), ver(GAME_VERSION), true);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), u"2.0.0-pre.8"_s);
    }

    void malformedReleaseIsSkippedButScanContinues_data() {
        QTest::addColumn<QJsonObject>("badRelease");
        QTest::newRow("modversion missing") << withoutKey(release(u"9.9.9"_s, {u"1.22.0"_s}), "modversion"_L1);
        QTest::newRow("modversion not a string")
            << withKey(release(u"9.9.9"_s, {u"1.22.0"_s}), "modversion"_L1, QJsonValue{9});
        QTest::newRow("modversion unparsable")
            << withKey(release(u"9.9.9"_s, {u"1.22.0"_s}), "modversion"_L1, "not-a-version"_L1);
        QTest::newRow("tags not an array") << withKey(release(u"9.9.9"_s, {u"1.22.0"_s}), "tags"_L1, "1.22.0"_L1);
    }

    void malformedReleaseIsSkippedButScanContinues() {
        QFETCH(QJsonObject, badRelease);
        vsmm::ModEntry entry{localInfo()};
        const QJsonArray releases{badRelease, release(u"1.1.0"_s, {u"1.22.0"_s})};

        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), u"1.1.0"_s);
    }

    void missingReleasesLeaveNoUpdate_data() {
        QTest::addColumn<QJsonValue>("releases");
        QTest::newRow("missing") << QJsonValue{QJsonValue::Undefined};
        QTest::newRow("null") << QJsonValue{QJsonValue::Null};
        QTest::newRow("not an array") << QJsonValue{"1.1.0"_L1};
        QTest::newRow("empty array") << QJsonValue{QJsonArray{}};
    }

    void missingReleasesLeaveNoUpdate() {
        QFETCH(QJsonValue, releases);
        vsmm::ModEntry entry{localInfo()};

        entry.initOnlineInfo(withKey(modJson(), "releases"_L1, releases), ver(GAME_VERSION), false);

        QVERIFY(!entry.hasUpdate());
        // The rest of the online info must still land.
        QCOMPARE(entry.getName(), u"Carry On"_s);
    }

    void newerReleaseBehindAnOlderOneIsStillFound() {
        vsmm::ModEntry entry{localInfo()};
        // The API orders releases by release date, so an older release can sit in front of a newer one.
        const QJsonArray releases{release(u"1.0.0"_s, {u"1.22.0"_s}), release(u"2.0.0"_s, {u"1.22.0"_s})};

        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), u"2.0.0"_s);
    }

    void dateOrderedReleasesResolveToTheHighestVersion() {
        // Shape taken from the live /api/mod/carryon response: a 1.x maintenance release is published
        // after the 2.0 prereleases, so it comes first in the array.
        vsmm::ModEntry entry{localInfo(u"2.0.0-pre.1"_s)};
        const QJsonArray releases{release(u"1.14.3"_s, {u"1.22.0"_s}), release(u"2.0.0-pre.8"_s, {u"1.22.0"_s}),
                                  release(u"2.0.0-pre.7"_s, {u"1.22.0"_s})};

        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), u"2.0.0-pre.8"_s);
        QCOMPARE(entry.getLatestVersion().mFileName, u"CarryOn_v2.0.0-pre.8.zip"_s);
    }

    void incompatibleNewerReleaseFallsBackToAnOlderCompatibleOne() {
        vsmm::ModEntry entry{localInfo()};
        // Newest release dropped support for the installed game version, the previous one still has it.
        const QJsonArray releases{release(u"2.0.0"_s, {u"1.23.0"_s}), release(u"1.5.0"_s, {u"1.22.0"_s})};

        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), u"1.5.0"_s);
    }

    void unsortedReleasesResolveToTheHighestCompatibleVersion() {
        // Terra Prety: a 6.x release published after the 7.x line, and 7.8.2 published after 7.9.2.
        vsmm::ModEntry entry{localInfo(u"7.9.2"_s)};
        const QJsonArray releases{release(u"6.2.0"_s, {u"1.22.2"_s}), release(u"7.10.2"_s, {u"1.22.0"_s}),
                                  release(u"7.8.2"_s, {u"1.22.0"_s}), release(u"7.10.1"_s, {u"1.22.0"_s})};

        entry.initOnlineInfo(modJson(releases), ver(GAME_VERSION), false);

        QVERIFY(entry.hasUpdate());
        QCOMPARE(str(entry.getLatestVersion().mVersion), u"7.10.2"_s);
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
