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

#include <ZipArchive.hpp>

#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QThreadPool>
#include <zip.h>

using namespace Qt::StringLiterals;

namespace {

struct Entry {
    QByteArray mName;
    QByteArray mContent;
};

// fixtures are built with libzip itself, so no binary archive has to be checked in
[[nodiscard]] bool writeArchive(const QString &file, const QList<Entry> &entries, const QByteArrayList &dirs = {}) {
    int errorCode{-1};
    zip_t *archive = zip_open(file.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
    if (!archive) {
        return false;
    }

    for (const auto &[name, content] : entries) {
        // the source borrows the buffer, entries outlives zip_close as the caller's argument
        zip_source_t *source = zip_source_buffer(archive, content.constData(), content.size(), 0);
        if (!source || zip_file_add(archive, name.constData(), source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) < 0) {
            zip_source_free(source);
            zip_discard(archive);
            return false;
        }
    }

    for (const auto &dir : dirs) {
        if (zip_dir_add(archive, dir.constData(), ZIP_FL_ENC_UTF_8) < 0) {
            zip_discard(archive);
            return false;
        }
    }

    return zip_close(archive) == 0;
}

// an encrypted entry stats fine but cannot be opened without the password, which no other fixture reaches.
// libzip can be built without crypto, so the caller has to treat a false here as "not testable on this build"
[[nodiscard]] bool writeEncryptedArchive(const QString &file, const Entry &entry) {
    int errorCode{-1};
    zip_t *archive = zip_open(file.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
    if (!archive) {
        return false;
    }

    zip_source_t *source = zip_source_buffer(archive, entry.mContent.constData(), entry.mContent.size(), 0);
    const zip_int64_t index =
        source ? zip_file_add(archive, entry.mName.constData(), source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) : -1;
    if (index < 0) {
        zip_source_free(source);
        zip_discard(archive);
        return false;
    }
    if (zip_file_set_encryption(archive, index, ZIP_EM_AES_256, "password") < 0) {
        zip_discard(archive);
        return false;
    }
    return zip_close(archive) == 0;
}

// local file header layout, only what is needed to find the compressed data of the first entry
constexpr qint64 LOCAL_HEADER_SIZE = 30;
constexpr int COMPRESSED_SIZE_OFFSET = 18;
constexpr int NAME_LENGTH_OFFSET = 26;
constexpr int EXTRA_LENGTH_OFFSET = 28;

[[nodiscard]] quint32 leNumber(const QByteArray &header, int at, int bytes) {
    quint32 value{0};
    for (int i = 0; i < bytes; ++i) {
        value |= static_cast<quint32>(static_cast<quint8>(header.at(at + i))) << (8 * i);
    }
    return value;
}

// overwrites the middle of the first entry's deflate stream, so inflating it stops short
[[nodiscard]] bool corruptFirstEntryData(const QString &file) {
    QFile archive{file};
    if (!archive.open(QIODevice::ReadWrite)) {
        return false;
    }

    const QByteArray header = archive.read(LOCAL_HEADER_SIZE);
    if (header.size() != LOCAL_HEADER_SIZE) {
        return false;
    }

    const quint32 compressedSize = leNumber(header, COMPRESSED_SIZE_OFFSET, 4);
    const qint64 dataStart =
        LOCAL_HEADER_SIZE + leNumber(header, NAME_LENGTH_OFFSET, 2) + leNumber(header, EXTRA_LENGTH_OFFSET, 2);
    const QByteArray junk{8, '\xFF'};
    if (compressedSize < 4 * junk.size() || !archive.seek(dataStart + compressedSize / 2)) {
        return false;
    }
    return archive.write(junk) == junk.size();
}

// every failure message ends in libzip's own text and code, which is its business to word.
// the helpers below build only our half of it, so the assertions pin what this class controls
[[nodiscard]] QString openFailure(const QString &file) { return u"Failed to open archive %1: "_s.arg(file); }

[[nodiscard]] QString lookupFailure(QUtf8StringView name, const QString &file) {
    return u"Failed to locate entry %1 in archive %2: "_s.arg(QString::fromUtf8(name.data(), name.size()), file);
}

[[nodiscard]] QString statFailure(vsmm::ZipArchive::FileIndex index, const QString &file) {
    return u"Failed to stat entry %1 in archive %2: "_s.arg(index).arg(file);
}

[[nodiscard]] QString openEntryFailure(vsmm::ZipArchive::FileIndex index, const QString &file) {
    return u"Failed to open entry %1 in archive %2: "_s.arg(index).arg(file);
}

[[nodiscard]] QString shortRead(vsmm::ZipArchive::FileIndex index, const QString &file) {
    return u"Short read of entry %1 in archive %2: "_s.arg(index).arg(file);
}

// the only message with no libzip half, so it is matched whole
[[nodiscard]] QString notOpen(const QString &file) { return u"Archive %1 is not open"_s.arg(file); }

// the message of a call that had to fail, so an unexpected success reads as a mismatch instead of throwing
template <typename T> [[nodiscard]] QString failureOf(const std::expected<T, QString> &result) {
    return result ? u"unexpectedly succeeded"_s : result.error();
}

// most functions only care about the bytes, the lookup and read failures have their own coverage
[[nodiscard]] QByteArray contentOf(const vsmm::ZipArchive &archive, QUtf8StringView name) {
    const auto index = archive.getFileIndex(name);
    return index ? archive.getFileContent(*index).value_or(QByteArray{}) : QByteArray{};
}

constexpr auto MOD_INFO = "modinfo.json";
constexpr auto MOD_INFO_CONTENT = R"({"modid": "carryon", "version": "1.0.0"})";
constexpr auto REPLACED_CONTENT = R"({"modid": "carryon", "version": "0.9.0"})";

[[nodiscard]] QList<Entry> defaultEntries() {
    return {{.mName = MOD_INFO, .mContent = MOD_INFO_CONTENT}, {.mName = "modicon.png", .mContent = "not-a-png"}};
}

} // namespace

class ZipArchiveIntegrationTest : public QObject {
    Q_OBJECT

    QTemporaryDir mDir;

    [[nodiscard]] QString filePath(QLatin1StringView name) const { return mDir.filePath(name); }

    // takes both sides as parameters, so a self-move reaches operator= without tripping -Wself-move
    static void moveOnto(vsmm::ZipArchive &target, vsmm::ZipArchive &&source) { target = std::move(source); }

    // one archive per test function, so no function depends on what another one wrote
    [[nodiscard]] QString writeDefaultArchive(QLatin1StringView name) const {
        const QString file = filePath(name);
        if (!writeArchive(file, defaultEntries())) {
            QTest::qVerify(false, "writeArchive(file, defaultEntries())", qPrintable(file), __FILE__, __LINE__);
            return {};
        }
        return file;
    }

  private slots:
    void initTestCase() { QVERIFY2(mDir.isValid(), qPrintable(mDir.errorString())); }

    // open

    void openReportsSuccessForAValidArchive() {
        vsmm::ZipArchive archive{writeDefaultArchive("valid.zip"_L1)};

        const auto opened = archive.open();

        QVERIFY2(opened.has_value(), qPrintable(failureOf(opened)));
    }

    void openIsIdempotent() {
        vsmm::ZipArchive archive{writeDefaultArchive("idempotent.zip"_L1)};
        QVERIFY(archive.open());

        QVERIFY(archive.open());
        // the first handle is still the live one
        QVERIFY(archive.getFileIndex(MOD_INFO));
    }

    void openReportsAMissingFile() {
        const QString file = filePath("missing.zip"_L1);
        QVERIFY(!QFile::exists(file));
        vsmm::ZipArchive archive{file};

        const QString error = failureOf(archive.open());

        QVERIFY2(error.startsWith(openFailure(file)), qPrintable(error));
        // the code is the one part of libzip's half worth pinning, a missing file is not ambiguous
        QVERIFY2(error.endsWith(u"(%1)"_s.arg(ZIP_ER_NOENT)), qPrintable(error));
    }

    void openReportsAFileThatIsNotAnArchive() {
        const QString file = filePath("garbage.zip"_L1);
        QFile garbage{file};
        QVERIFY(garbage.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(garbage.write("this is not a zip"), 17);
        garbage.close();
        vsmm::ZipArchive archive{file};

        // the code libzip picks for junk is its business, only the report and the verdict are ours
        const QString error = failureOf(archive.open());

        QVERIFY2(error.startsWith(openFailure(file)), qPrintable(error));
    }

    void openIsRetriedAfterAFailure() {
        const QString file = filePath("appears-later.zip"_L1);
        vsmm::ZipArchive archive{file};
        QVERIFY(!archive.open());

        QVERIFY(writeArchive(file, defaultEntries()));

        QVERIFY(archive.open());
        QVERIFY(archive.getFileIndex(MOD_INFO));
    }

    // entry lookup

    void findsAnEntryByItsExactName() {
        vsmm::ZipArchive archive{writeDefaultArchive("lookup.zip"_L1)};
        QVERIFY(archive.open());

        const auto modInfo = archive.getFileIndex(MOD_INFO);
        const auto icon = archive.getFileIndex("modicon.png");

        QVERIFY2(modInfo.has_value(), qPrintable(failureOf(modInfo)));
        QVERIFY2(icon.has_value(), qPrintable(failureOf(icon)));
        QVERIFY(*modInfo != *icon);
    }

    void lookupMissesAreReported_data() {
        QTest::addColumn<QByteArray>("name");

        QTest::newRow("absent-entry") << QByteArray{"assets/nothing.json"};
        QTest::newRow("empty-name") << QByteArray{};
        // the case rows pin ZIP_FL_NOCASE staying off
        QTest::newRow("mixed-case") << QByteArray{"ModInfo.json"};
        QTest::newRow("upper-case") << QByteArray{"MODINFO.JSON"};
    }

    void lookupMissesAreReported() {
        QFETCH(QByteArray, name);
        const QString file = writeDefaultArchive("lookup-miss.zip"_L1);
        vsmm::ZipArchive archive{file};
        QVERIFY(archive.open());

        const QString error = failureOf(archive.getFileIndex(name.constData()));

        QVERIFY2(error.startsWith(lookupFailure(name.constData(), file)), qPrintable(error));
    }

    void aNestedEntryNeedsItsFullPath() {
        const QString file = filePath("nested.zip"_L1);
        QVERIFY(writeArchive(file, {{.mName = "assets/modinfo.json", .mContent = MOD_INFO_CONTENT}}));
        vsmm::ZipArchive archive{file};
        QVERIFY(archive.open());

        QVERIFY(archive.getFileIndex("assets/modinfo.json"));

        // a mod that buries modinfo.json in a subdirectory is not found, which is what ModLoader reports
        const QString error = failureOf(archive.getFileIndex(MOD_INFO));
        QVERIFY2(error.startsWith(lookupFailure(MOD_INFO, file)), qPrintable(error));
    }

    void findsANonAsciiEntryName() {
        const QString file = filePath("non-ascii.zip"_L1);
        // "mødinfo.json" spelled in explicit utf-8 bytes, the escape is split so the d is not eaten by \x
        constexpr auto name = "m\xC3\xB8"
                              "dinfo.json";
        QVERIFY(writeArchive(file, {{.mName = name, .mContent = MOD_INFO_CONTENT}}));
        vsmm::ZipArchive archive{file};
        QVERIFY(archive.open());

        const auto index = archive.getFileIndex(name);

        QVERIFY2(index.has_value(), qPrintable(failureOf(index)));
        QCOMPARE(archive.getFileContent(*index).value_or(QByteArray{}), MOD_INFO_CONTENT);
    }

    void anUnopenedArchiveIsSafeToUse() {
        const QString file = writeDefaultArchive("unopened.zip"_L1);
        const vsmm::ZipArchive archive{file};

        // the guards are the difference between a report and a zip_stat_index crash on a null handle
        QCOMPARE(failureOf(archive.getFileIndex(MOD_INFO)), notOpen(file));
        QCOMPARE(failureOf(archive.getFileContent(0)), notOpen(file));
    }

    // entry content

    void readsAnEntryVerbatim() {
        vsmm::ZipArchive archive{writeDefaultArchive("content.zip"_L1)};
        QVERIFY(archive.open());

        QCOMPARE(contentOf(archive, MOD_INFO), MOD_INFO_CONTENT);
        QCOMPARE(contentOf(archive, "modicon.png"), "not-a-png");
    }

    void readsBinaryContentVerbatim() {
        const QString file = filePath("binary.zip"_L1);
        const QByteArray content = QByteArray::fromHex("89504e470d0a1a0a0000000d49484452"_ba) + QByteArray(3, '\0');
        QVERIFY(writeArchive(file, {{.mName = "modicon.png", .mContent = content}}));
        vsmm::ZipArchive archive{file};
        QVERIFY(archive.open());

        const QByteArray read = contentOf(archive, "modicon.png");

        // embedded NULs must survive, the buffer is sized from the stat and not from strlen
        QCOMPARE(read.size(), content.size());
        QCOMPARE(read, content);
    }

    void readsAnEntryLargerThanItsCompressedSize() {
        const QString file = filePath("large.zip"_L1);
        const QByteArray content = QByteArray{1024 * 1024, 'x'}.append("tail"_ba);
        QVERIFY(writeArchive(file, {{.mName = "assets/big.json", .mContent = content}}));
        vsmm::ZipArchive archive{file};
        QVERIFY(archive.open());

        const QByteArray read = contentOf(archive, "assets/big.json");

        QCOMPARE(read.size(), content.size());
        QCOMPARE(read, content);
    }

    void entriesWithNoBytesReadAsEmpty() {
        const QString file = filePath("no-bytes.zip"_L1);
        QVERIFY(writeArchive(file, {{.mName = MOD_INFO, .mContent = {}}}, {"assets/"}));
        vsmm::ZipArchive archive{file};
        QVERIFY(archive.open());

        const auto modInfo = archive.getFileIndex(MOD_INFO);
        const auto directory = archive.getFileIndex("assets/");
        QVERIFY2(modInfo.has_value(), qPrintable(failureOf(modInfo)));
        QVERIFY2(directory.has_value(), qPrintable(failureOf(directory)));

        // both read back as an empty success, whether that is usable is ModLoader's judgement and not this class's
        const auto empty = archive.getFileContent(*modInfo);
        const auto listing = archive.getFileContent(*directory);

        QVERIFY2(empty.has_value(), qPrintable(failureOf(empty)));
        QVERIFY(empty->isEmpty());
        QVERIFY2(listing.has_value(), qPrintable(failureOf(listing)));
        QVERIFY(listing->isEmpty());
    }

    void aShortReadIsReported() {
        const QString file = filePath("corrupt.zip"_L1);
        // periodic enough to be deflated, long enough that the stream has a middle to damage
        const QByteArray content = QByteArray{"abcdefghijklmnopq"_ba}.repeated(4096);
        QVERIFY(writeArchive(file, {{.mName = "assets/big.json", .mContent = content}}));
        QVERIFY(corruptFirstEntryData(file));
        vsmm::ZipArchive archive{file};
        QVERIFY(archive.open());
        const auto index = archive.getFileIndex("assets/big.json");
        QVERIFY2(index.has_value(), qPrintable(failureOf(index)));

        // how far inflate gets before it gives up is zlib's business, the report and the verdict are ours
        const QString error = failureOf(archive.getFileContent(*index));

        QVERIFY2(error.startsWith(shortRead(*index, file)), qPrintable(error));
    }

    void unusableIndexesAreReported() {
        const QString file = writeDefaultArchive("bad-index.zip"_L1);
        vsmm::ZipArchive archive{file};
        QVERIFY(archive.open());

        const QString beyondTheEnd = failureOf(archive.getFileContent(999));
        QVERIFY2(beyondTheEnd.startsWith(statFailure(999, file)), qPrintable(beyondTheEnd));

        // -1 is what a failed lookup used to hand over, so it stays covered even now that it cannot reach here
        const QString negative = failureOf(archive.getFileContent(-1));
        QVERIFY2(negative.startsWith(statFailure(-1, file)), qPrintable(negative));
    }

    void readingTheSameEntryTwiceYieldsTheSameBytes() {
        vsmm::ZipArchive archive{writeDefaultArchive("twice.zip"_L1)};
        QVERIFY(archive.open());
        const auto index = archive.getFileIndex(MOD_INFO);
        QVERIFY2(index.has_value(), qPrintable(failureOf(index)));

        // each read opens and closes its own entry handle, so nothing is consumed by the first one
        QCOMPARE(archive.getFileContent(*index).value_or(QByteArray{}), MOD_INFO_CONTENT);
        QCOMPARE(archive.getFileContent(*index).value_or(QByteArray{}), MOD_INFO_CONTENT);
    }

    void anEntryThatCannotBeOpenedIsReported() {
        const QString file = filePath("encrypted.zip"_L1);
        if (!writeEncryptedArchive(file, {.mName = MOD_INFO, .mContent = MOD_INFO_CONTENT})) {
            QSKIP("This libzip build cannot write an encrypted entry");
        }
        vsmm::ZipArchive archive{file};
        QVERIFY(archive.open());
        const auto index = archive.getFileIndex(MOD_INFO);
        QVERIFY2(index.has_value(), qPrintable(failureOf(index)));

        // the stat succeeds, only the read wants the password that is never passed
        const QString error = failureOf(archive.getFileContent(*index));

        QVERIFY2(error.startsWith(openEntryFailure(*index, file)), qPrintable(error));
    }

    void concurrentReadersOfOneArchiveAreIndependent() {
        const QString file = writeDefaultArchive("concurrent.zip"_L1);
        // ModLoader extracts on a pool of 4, each task owning its own archive over its own file
        constexpr int READERS = 8;
        QAtomicInt reads{0};
        QThreadPool pool;
        pool.setMaxThreadCount(READERS);

        for (int i = 0; i < READERS; ++i) {
            pool.start([file, &reads] {
                vsmm::ZipArchive archive{file};
                if (archive.open() && contentOf(archive, MOD_INFO) == MOD_INFO_CONTENT) {
                    reads.fetchAndAddRelaxed(1);
                }
            });
        }

        QVERIFY(pool.waitForDone(10'000));
        QCOMPARE(reads.loadRelaxed(), READERS);
    }

    // move semantics

    void moveConstructionTakesOverAnUnopenedArchive() {
        vsmm::ZipArchive source{writeDefaultArchive("moved-ctor.zip"_L1)};

        vsmm::ZipArchive moved{std::move(source)};

        QVERIFY(moved.open());
        QVERIFY(moved.getFileIndex(MOD_INFO));
    }

    void moveAssignmentTakesOverAnUnopenedArchive() {
        vsmm::ZipArchive source{writeDefaultArchive("moved-assign.zip"_L1)};
        vsmm::ZipArchive target{filePath("assign-target.zip"_L1)};

        target = std::move(source);

        QVERIFY(target.open());
        QVERIFY(target.getFileIndex(MOD_INFO));
    }

    void moveConstructionCarriesTheOpenHandle() {
        vsmm::ZipArchive source{writeDefaultArchive("moved-open-ctor.zip"_L1)};
        QVERIFY(source.open());

        vsmm::ZipArchive moved{std::move(source)};

        // the handle moves with the path, so no reopen is needed
        QCOMPARE(contentOf(moved, MOD_INFO), MOD_INFO_CONTENT);
    }

    void moveAssignmentCarriesTheOpenHandle() {
        vsmm::ZipArchive source{writeDefaultArchive("moved-open-assign.zip"_L1)};
        QVERIFY(source.open());
        vsmm::ZipArchive target{filePath("assign-open-target.zip"_L1)};

        target = std::move(source);

        QCOMPARE(contentOf(target, MOD_INFO), MOD_INFO_CONTENT);
    }

    void moveAssignmentSwapsTheTwoArchives() {
        const QString replaced = filePath("replaced.zip"_L1);
        QVERIFY(writeArchive(replaced, {{.mName = MOD_INFO, .mContent = REPLACED_CONTENT}}));
        vsmm::ZipArchive target{replaced};
        QVERIFY(target.open());
        vsmm::ZipArchive source{writeDefaultArchive("replacement.zip"_L1)};
        QVERIFY(source.open());

        target = std::move(source);

        QCOMPARE(contentOf(target, MOD_INFO), MOD_INFO_CONTENT);
        // NOLINTBEGIN(bugprone-use-after-move) the target's old archive lives on in the source until it dies
        QCOMPARE(contentOf(source, MOD_INFO), REPLACED_CONTENT);
        // NOLINTEND(bugprone-use-after-move)
    }

    void selfMoveAssignmentIsANoOp() {
        vsmm::ZipArchive archive{writeDefaultArchive("self-move.zip"_L1)};
        QVERIFY(archive.open());

        // routed through a helper, a literal archive = std::move(archive) is a compiler diagnostic
        moveOnto(archive, std::move(archive));

        QCOMPARE(contentOf(archive, MOD_INFO), MOD_INFO_CONTENT);
    }

    void aMovedFromArchiveIsLeftClosed() {
        vsmm::ZipArchive source{writeDefaultArchive("moved-from.zip"_L1)};
        QVERIFY(source.open());

        const vsmm::ZipArchive moved{std::move(source)};

        // NOLINTBEGIN(bugprone-use-after-move) the moved-from archive must stay safe to touch
        // a moved-from QString is valid but unspecified, so the path in the report stays unpinned
        const QString lookup = failureOf(source.getFileIndex(MOD_INFO));
        QVERIFY2(lookup.endsWith(u" is not open"_s), qPrintable(lookup));
        const QString read = failureOf(source.getFileContent(0));
        QVERIFY2(read.endsWith(u" is not open"_s), qPrintable(read));
        // NOLINTEND(bugprone-use-after-move)
    }
};

QTEST_GUILESS_MAIN(ZipArchiveIntegrationTest)
#include "ZipArchiveIntegrationTest.moc"
