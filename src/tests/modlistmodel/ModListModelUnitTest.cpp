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

#include <ModEntryTestUtils.hpp>
#include <ModListModel.hpp>
#include <ModStoreMock.hpp>

#include <QAbstractItemModelTester>
#include <QLoggingCategory>
#include <QSignalSpy>
#include <QStringListModel>
#include <QTest>

#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;
using Roles = vsmm::ModListModel::Roles;

namespace {
constexpr auto GAME_VERSION = "1.22.5";

// the mock never touches the file, so no zip behind it
[[nodiscard]] vsmm::ModEntry mod(const QString &id, const QString &version = u"1.0.0"_s) {
    return vsmm::ModEntry{localInfo(id, version, QFileInfo{})};
}

// first and last row of a rowsInserted/rowsRemoved emission, parent must be the root
[[nodiscard]] QPair<int, int> rowRange(const QList<QVariant> &args) {
    QTest::qVerify(!args.at(0).value<QModelIndex>().isValid(), "parent is root", "", __FILE__, __LINE__);
    return {args.at(1).toInt(), args.at(2).toInt()};
}

// top-left and bottom-right row of a dataChanged emission
[[nodiscard]] QPair<int, int> changedRows(const QList<QVariant> &args) {
    return {args.at(0).value<QModelIndex>().row(), args.at(1).value<QModelIndex>().row()};
}
} // namespace

class ModListModelUnitTest : public QObject {
    Q_OBJECT

    // only this component logs, and no tracing unless a test asks for it
    static void onlyModelLogs() {
        QLoggingCategory::setFilterRules(u"*=false\nmodlistmodel=true\nmodlistmodel.debug=false"_s);
    }
    static void enableModelTracing() { QLoggingCategory::setFilterRules(u"*=false\nmodlistmodel=true"_s); }

    std::unique_ptr<vsmm::ModStoreMock> mStore;
    std::unique_ptr<vsmm::ModListModel> mModel;
    std::unique_ptr<QAbstractItemModelTester> mTester;

    [[nodiscard]] QVariant role(int row, int role) const { return mModel->data(mModel->index(row), role); }

    // what the view would show, top to bottom
    [[nodiscard]] QStringList ids() const {
        QStringList ids;
        for (int row = 0; row < mModel->rowCount(QModelIndex{}); ++row) {
            ids.append(role(row, Roles::IdRole).toString());
        }
        return ids;
    }

    void addThree() {
        mStore->add(mod(u"carryon"_s));
        mStore->add(mod(u"betterruins"_s));
        mStore->add(mod(u"animalcages"_s));
    }

    void applyOnline(const QString &id, const QJsonObject &json) {
        mStore->entry(id).initOnlineInfo(json, ver(QString::fromLatin1(GAME_VERSION)), false);
        mStore->update(id);
    }

  private slots:
    void init() {
        onlyModelLogs();

        mStore = std::make_unique<vsmm::ModStoreMock>();
        mModel = std::make_unique<vsmm::ModListModel>();
        mModel->setStore(mStore.get());
        // checks every insert, removal and data() call below against the QAbstractItemModel contract
        mTester = std::make_unique<QAbstractItemModelTester>(mModel.get(),
                                                             QAbstractItemModelTester::FailureReportingMode::QtTest);
    }

    void cleanup() {
        mTester.reset();
        mModel.reset();
        mStore.reset();
    }

    void freshModelIsEmpty() {
        QCOMPARE(mModel->rowCount(QModelIndex{}), 0);

        mStore->add(mod(u"carryon"_s));

        QCOMPARE(mModel->rowCount(QModelIndex{}), 1);
        // a flat list, rows have no children
        QCOMPARE(mModel->rowCount(mModel->index(0)), 0);
    }

    void modelWithoutAStoreReturnsNothing() {
        const vsmm::ModListModel model;

        QCOMPARE(model.rowCount(QModelIndex{}), 0);
        QVERIFY(!model.data(QModelIndex{}, Roles::NameRole).isValid());
    }

    // set-once: a second call must not connect the other store, or rows would arrive from it
    void secondStoreIsRefused() {
        vsmm::ModStoreMock other;

        QTest::ignoreMessage(QtWarningMsg, "ModStore already set");
        mModel->setStore(&other);

        other.add(mod(u"ghost"_s));
        QCOMPARE(mModel->rowCount(QModelIndex{}), 0);

        mStore->add(mod(u"carryon"_s));
        QCOMPARE(ids(), QStringList{u"carryon"_s});
    }

    void roleNamesMatchTheQmlContract_data() {
        QTest::addColumn<int>("role");
        QTest::addColumn<QByteArray>("name");

        QTest::newRow("NameRole") << int{Roles::NameRole} << "modName"_ba;
        QTest::newRow("AuthorRole") << int{Roles::AuthorRole} << "modAuthor"_ba;
        QTest::newRow("VersionRole") << int{Roles::VersionRole} << "modVersion"_ba;
        QTest::newRow("LatestVersionRole") << int{Roles::LatestVersionRole} << "modLatestVersion"_ba;
        QTest::newRow("TagsRole") << int{Roles::TagsRole} << "modTags"_ba;
        QTest::newRow("UrlRole") << int{Roles::UrlRole} << "modUrl"_ba;
        QTest::newRow("TypeRole") << int{Roles::TypeRole} << "modSide"_ba;
        QTest::newRow("HasUpdateRole") << int{Roles::HasUpdateRole} << "modHasUpdate"_ba;
        QTest::newRow("IconRole") << int{Roles::IconRole} << "modThumbnail"_ba;
        QTest::newRow("IdRole") << int{Roles::IdRole} << "modId"_ba;
        QTest::newRow("FavoriteRole") << int{Roles::FavoriteRole} << "isFavoriteMod"_ba;
    }

    // the delegates bind to these names, a rename breaks qml silently
    void roleNamesMatchTheQmlContract() {
        QFETCH(const int, role);
        QFETCH(const QByteArray, name);

        QCOMPARE(mModel->roleNames().value(role), name);
    }

    void everyRoleHasAName() {
        QCOMPARE(static_cast<int>(mModel->roleNames().size()), Roles::FavoriteRole - Roles::NameRole + 1);
    }

    // sorting belongs to the proxy, the model keeps scan order
    void addedModsKeepScanOrder() {
        const QSignalSpy inserted{mModel.get(), &QAbstractItemModel::rowsInserted};

        mStore->add(mod(u"zebrautils"_s));
        mStore->add(mod(u"animalcages"_s));
        mStore->add(mod(u"moreruins"_s));

        QCOMPARE(ids(), QStringList({u"zebrautils"_s, u"animalcages"_s, u"moreruins"_s}));
        QCOMPARE(inserted.count(), 3);
        for (int row = 0; row < 3; ++row) {
            QCOMPARE(rowRange(inserted.at(row)), qMakePair(row, row));
        }
    }

    // ModStore never announces an id twice, the guard keeps a stray one from duplicating the row
    void duplicateAddIsIgnored() {
        mStore->add(mod(u"carryon"_s));
        const QSignalSpy inserted{mModel.get(), &QAbstractItemModel::rowsInserted};

        enableModelTracing();
        QTest::ignoreMessage(QtDebugMsg, "Mod carryon is already in the model");
        emit mStore->modAdded(u"carryon"_s);

        QCOMPARE(inserted.count(), 0);
        QCOMPARE(ids(), QStringList{u"carryon"_s});
    }

    // a newer zip of a known mod replaces the entry, the row stays and only refreshes
    void replacedModUpdatesItsRowInPlace() {
        addThree();
        const QSignalSpy inserted{mModel.get(), &QAbstractItemModel::rowsInserted};
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        mStore->add(mod(u"betterruins"_s, u"1.1.0"_s));

        QCOMPARE(inserted.count(), 0);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changedRows(changed.at(0)), qMakePair(1, 1));
        QCOMPARE(ids(), QStringList({u"carryon"_s, u"betterruins"_s, u"animalcages"_s}));
        QCOMPARE(role(1, Roles::VersionRole).toString(), u"1.1.0"_s);
    }

    void removeDropsOnlyThatRow_data() {
        QTest::addColumn<QString>("removed");
        QTest::addColumn<int>("row");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("first") << u"carryon"_s << 0 << QStringList({u"betterruins"_s, u"animalcages"_s});
        QTest::newRow("middle") << u"betterruins"_s << 1 << QStringList({u"carryon"_s, u"animalcages"_s});
        QTest::newRow("last") << u"animalcages"_s << 2 << QStringList({u"carryon"_s, u"betterruins"_s});
    }

    void removeDropsOnlyThatRow() {
        QFETCH(const QString, removed);
        QFETCH(const int, row);
        QFETCH(const QStringList, expected);
        addThree();
        const QSignalSpy removedRows{mModel.get(), &QAbstractItemModel::rowsRemoved};

        mStore->remove(removed);

        QCOMPARE(ids(), expected);
        QCOMPARE(removedRows.count(), 1);
        QCOMPARE(rowRange(removedRows.at(0)), qMakePair(row, row));
    }

    // rows below a removal shift up, a stale id-to-row map would point updates and removals at the wrong row
    void removeReindexesTheRowsBelow() {
        addThree();
        mStore->remove(u"betterruins"_s);
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};
        const QSignalSpy removedRows{mModel.get(), &QAbstractItemModel::rowsRemoved};

        mStore->update(u"animalcages"_s);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changedRows(changed.at(0)), qMakePair(1, 1));

        mStore->remove(u"animalcages"_s);
        QCOMPARE(removedRows.count(), 1);
        QCOMPARE(rowRange(removedRows.at(0)), qMakePair(1, 1));
        QCOMPARE(ids(), QStringList{u"carryon"_s});
    }

    void removalOfUnknownIdIsIgnored() {
        mStore->add(mod(u"carryon"_s));
        const QSignalSpy removing{mModel.get(), &QAbstractItemModel::rowsAboutToBeRemoved};

        enableModelTracing();
        QTest::ignoreMessage(QtDebugMsg, "Removal of mod ghost outside the model");
        emit mStore->modRemoved(u"ghost"_s);

        QCOMPARE(removing.count(), 0);
        QCOMPARE(ids(), QStringList{u"carryon"_s});
    }

    // one bulk removal, not one per row
    void reloadDropsEveryRow() {
        addThree();
        const QSignalSpy removedRows{mModel.get(), &QAbstractItemModel::rowsRemoved};

        mStore->reload();

        QCOMPARE(mModel->rowCount(QModelIndex{}), 0);
        QCOMPARE(removedRows.count(), 1);
        QCOMPARE(rowRange(removedRows.at(0)), qMakePair(0, 2));
    }

    // beginRemoveRows(0, -1) would be a contract violation
    void reloadOfAnEmptyModelEmitsNothing() {
        const QSignalSpy removing{mModel.get(), &QAbstractItemModel::rowsAboutToBeRemoved};

        mStore->reload();

        QCOMPARE(removing.count(), 0);
    }

    // a rescan brings back the same ids, a leftover id-to-row entry would reject them as duplicates
    void modsCanBeReaddedAfterAReload() {
        mStore->add(mod(u"carryon"_s));
        mStore->add(mod(u"betterruins"_s));
        mStore->reload();

        mStore->add(mod(u"betterruins"_s));
        mStore->add(mod(u"carryon"_s));

        QCOMPARE(ids(), QStringList({u"betterruins"_s, u"carryon"_s}));
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};
        mStore->update(u"carryon"_s);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changedRows(changed.at(0)), qMakePair(1, 1));
    }

    void tracingReportsRowChanges() {
        enableModelTracing();

        QTest::ignoreMessage(QtDebugMsg, "Mod carryon inserted at row 0");
        QTest::ignoreMessage(QtDebugMsg, "Mod betterruins inserted at row 1");
        QTest::ignoreMessage(QtDebugMsg, "Mod animalcages inserted at row 2");
        addThree();

        QTest::ignoreMessage(QtDebugMsg, "Mod carryon removed from row 0");
        mStore->remove(u"carryon"_s);

        QTest::ignoreMessage(QtDebugMsg, "Dropping 2 rows for reload");
        mStore->reload();
    }

    void localOnlyModShowsLocalInfo() {
        mStore->add(mod(u"carryon"_s, u"1.2.3"_s));

        QCOMPARE(role(0, Roles::NameRole).toString(), u"carryon (local)"_s);
        QCOMPARE(role(0, Roles::AuthorRole).toString(), u"local author"_s);
        QCOMPARE(role(0, Roles::VersionRole).toString(), u"1.2.3"_s);
        QCOMPARE(role(0, Roles::IdRole).toString(), u"carryon"_s);
        QVERIFY(role(0, Roles::TypeRole).toString().isEmpty());
        QVERIFY(role(0, Roles::TagsRole).toStringList().isEmpty());
        QVERIFY(role(0, Roles::UrlRole).toUrl().isEmpty());
        QCOMPARE(role(0, Roles::HasUpdateRole).toBool(), false);
        QCOMPARE(role(0, Roles::FavoriteRole).toBool(), false);
        // no online info yet, semver default, qml only shows it next to an update
        QCOMPARE(role(0, Roles::LatestVersionRole).toString(), str(semver::version<>{}));

        // qml binds it to a string property, so an empty string rather than an invalid variant
        const QVariant icon = role(0, Roles::IconRole);
        QCOMPARE(icon.typeId(), QMetaType::QString);
        QVERIFY(icon.toString().isEmpty());
    }

    void onlineInfoOverridesLocalInfo() {
        mStore->add(mod(u"carryon"_s));
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        applyOnline(u"carryon"_s, modJson({release(u"1.1.0"_s, {u"1.22.0"_s})}));

        QCOMPARE(changed.count(), 1);
        QCOMPARE(changedRows(changed.at(0)), qMakePair(0, 0));
        // no roles listed means every role, the delegate refetches the whole row
        QVERIFY(changed.at(0).at(2).value<QList<int>>().isEmpty());

        QCOMPARE(role(0, Roles::NameRole).toString(), u"Carry On"_s);
        QCOMPARE(role(0, Roles::AuthorRole).toString(), u"NerdScurvy"_s);
        QCOMPARE(role(0, Roles::VersionRole).toString(), u"1.0.0"_s);
        QCOMPARE(role(0, Roles::TypeRole).toString(), u"mod"_s);
        QCOMPARE(role(0, Roles::TagsRole).toStringList(), QStringList({u"Storage"_s, u"QoL"_s}));
        QCOMPARE(role(0, Roles::UrlRole).toUrl(), QUrl{u"https://mods.vintagestory.at/carryon"_s});
        QCOMPARE(role(0, Roles::LatestVersionRole).toString(), u"1.1.0"_s);
        QCOMPARE(role(0, Roles::HasUpdateRole).toBool(), true);
    }

    // same id format ModImageProvider parses, see iconId() in its test
    void iconRoleBuildsTheProviderId() {
        mStore->add(mod(u"carryon"_s));
        QJsonObject json = modJson();
        json.insert("logofile"_L1, u"https://mods.vintagestory.at/files/asset/4405/logo.png"_s);

        applyOnline(u"carryon"_s, json);

        QCOMPARE(role(0, Roles::IconRole).toString(),
                 u"image://modicon/carryon?url=https%3A%2F%2Fmods.vintagestory.at%2Ffiles%2Fasset%2F4405%2Flogo.png"_s);
    }

    void favoriteIsShown() {
        mStore->add(mod(u"carryon"_s));
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        mStore->entry(u"carryon"_s).setFavorite(true);
        mStore->update(u"carryon"_s);

        QCOMPARE(changed.count(), 1);
        QCOMPARE(role(0, Roles::FavoriteRole).toBool(), true);
    }

    void unknownRoleIsInvalid() {
        mStore->add(mod(u"carryon"_s));

        QVERIFY(!role(0, Qt::DisplayRole).isValid());
        QVERIFY(!role(0, Roles::FavoriteRole + 1).isValid());
    }

    void invalidIndexIsInvalid() {
        mStore->add(mod(u"carryon"_s));

        QVERIFY(!mModel->data(QModelIndex{}, Roles::NameRole).isValid());
    }

    // index() refuses rows past the end, only an index from another model gets that far
    void foreignRowBeyondTheModelIsInvalid() {
        mStore->add(mod(u"carryon"_s));
        const QStringListModel foreign{QStringList(6, u"x"_s)};
        const QModelIndex beyond = foreign.index(5);
        QVERIFY(beyond.isValid());

        QVERIFY(!mModel->data(beyond, Roles::NameRole).isValid());
    }

    // ModStore::reload empties the store before the model drops its rows, so rows can outlive their entry
    void rowWhoseModLeftTheStoreIsInvalid() {
        emit mStore->modAdded(u"ghost"_s);

        QCOMPARE(mModel->rowCount(QModelIndex{}), 1);
        QVERIFY(!role(0, Roles::NameRole).isValid());
        QVERIFY(!role(0, Roles::IdRole).isValid());
    }

    void updateOutsideTheModelIsIgnored() {
        mStore->add(mod(u"carryon"_s));
        const QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        enableModelTracing();
        QTest::ignoreMessage(QtDebugMsg, "Update for mod ghost outside the model");
        emit mStore->modUpdated(u"ghost"_s);

        QCOMPARE(changed.count(), 0);
    }
};

QTEST_GUILESS_MAIN(ModListModelUnitTest)
#include "ModListModelUnitTest.moc"
