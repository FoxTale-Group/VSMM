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
#include <ModStoreMock.hpp>
#include <ModUpdateModel.hpp>

#include <QAbstractItemModelTester>
#include <QLoggingCategory>
#include <QSignalSpy>
#include <QTest>

#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;
using Roles = vsmm::ModUpdateModel::Roles;
using State = vsmm::ModUpdateModel::State;

namespace {
constexpr auto GAME_VERSION = "1.22.5";

// installed 1.0.0, the API offers 1.1.0 for this game version
[[nodiscard]] vsmm::ModEntry modWithUpdate(const QString &id) {
    vsmm::ModEntry mod{localInfo(id, u"1.0.0"_s, QFileInfo{})};
    mod.initOnlineInfo(modJson({release(u"1.1.0"_s, {u"1.22.0"_s})}), ver(QString::fromLatin1(GAME_VERSION)), false);
    return mod;
}

// top-left row, bottom-right row and roles of a dataChanged emission
struct Change {
    int mFirst;
    int mLast;
    QList<int> mRoles;
};

[[nodiscard]] Change change(const QList<QVariant> &args) {
    return {args.at(0).value<QModelIndex>().row(), args.at(1).value<QModelIndex>().row(),
            args.at(2).value<QList<int>>()};
}
} // namespace

class ModUpdateModelUnitTest : public QObject {
    Q_OBJECT

    // only this component logs, and no tracing
    static void onlyModelLogs() {
        QLoggingCategory::setFilterRules(u"*=false\nmodupdatemodel=true\nmodupdatemodel.debug=false"_s);
    }

    std::unique_ptr<vsmm::ModStoreMock> mStore;
    std::unique_ptr<vsmm::ModUpdateModel> mModel;
    std::unique_ptr<QAbstractItemModelTester> mTester;

    [[nodiscard]] QVariant role(int row, int role) const { return mModel->data(mModel->index(row), role); }
    [[nodiscard]] State state(int row) const { return role(row, Roles::StateRole).value<State>(); }
    [[nodiscard]] double progress(int row) const { return role(row, Roles::ProgressRole).toDouble(); }

    [[nodiscard]] QStringList ids() const {
        QStringList ids;
        for (int row = 0; row < mModel->rowCount(QModelIndex{}); ++row) {
            ids.append(role(row, Roles::IdRole).toString());
        }
        return ids;
    }

    // carryon and betterruins, both outdated, requested as one batch
    void requestTwo() {
        mStore->add(modWithUpdate(u"carryon"_s));
        mStore->add(modWithUpdate(u"betterruins"_s));
        mStore->requestUpdate({u"carryon"_s, u"betterruins"_s});
    }

  private slots:
    void init() {
        onlyModelLogs();

        mStore = std::make_unique<vsmm::ModStoreMock>();
        mModel = std::make_unique<vsmm::ModUpdateModel>();
        mModel->setStore(mStore.get());
        // checks every reset and data() call below against the QAbstractItemModel contract
        mTester = std::make_unique<QAbstractItemModelTester>(mModel.get(),
                                                             QAbstractItemModelTester::FailureReportingMode::QtTest);
    }

    void cleanup() {
        mTester.reset();
        mModel.reset();
        mStore.reset();
    }

    void freshModelIsEmptyAndIdle() {
        QCOMPARE(mModel->rowCount(QModelIndex{}), 0);
        QCOMPARE(mModel->count(), 0);
        QCOMPARE(mModel->totalProgress(), 0.0);
        QVERIFY(!mModel->isActive());
    }

    void secondStoreIsRefused() {
        vsmm::ModStoreMock other;
        QTest::ignoreMessage(QtWarningMsg, "ModStore already set");

        mModel->setStore(&other);

        other.add(modWithUpdate(u"carryon"_s));
        other.requestUpdate({u"carryon"_s});
        QCOMPARE(mModel->count(), 0);
    }

    void roleNamesAreTheQmlNames() {
        const QHash<int, QByteArray> names = mModel->roleNames();

        QCOMPARE(names.value(Roles::IdRole), "modId"_ba);
        QCOMPARE(names.value(Roles::NameRole), "modName"_ba);
        QCOMPARE(names.value(Roles::FromVersionRole), "fromVersion"_ba);
        QCOMPARE(names.value(Roles::ToVersionRole), "toVersion"_ba);
        QCOMPARE(names.value(Roles::StateRole), "updateState"_ba);
        QCOMPARE(names.value(Roles::ProgressRole), "progress"_ba);
        QCOMPARE(names.value(Roles::ErrorRole), "errorText"_ba);
    }

    void outOfRangeRowAndUnknownRoleReturnNothing() {
        requestTwo();

        QVERIFY(!mModel->data(QModelIndex{}, Roles::IdRole).isValid());
        QVERIFY(!role(0, Qt::DisplayRole).isValid());
        // a flat list, rows have no children
        QCOMPARE(mModel->rowCount(mModel->index(0)), 0);

        // kept from before a smaller batch replaced the rows
        const QModelIndex stale = mModel->index(1);
        mStore->requestUpdate({u"carryon"_s});
        QVERIFY(!mModel->data(stale, Roles::IdRole).isValid());
    }

    void batchAddsOneQueuedRowPerModWithBothVersions() {
        QSignalSpy countChanged{mModel.get(), &vsmm::ModUpdateModel::countChanged};

        requestTwo();

        QCOMPARE(ids(), (QStringList{u"carryon"_s, u"betterruins"_s}));
        QCOMPARE(mModel->count(), 2);
        QCOMPARE(countChanged.count(), 1);
        QCOMPARE(role(0, Roles::NameRole).toString(), mStore->find(u"carryon"_s)->getName().toString());
        QCOMPARE(role(0, Roles::FromVersionRole).toString(), u"1.0.0"_s);
        QCOMPARE(role(0, Roles::ToVersionRole).toString(), u"1.1.0"_s);
        QCOMPARE(state(0), State::Queued);
        QCOMPARE(progress(0), 0.0);
        QVERIFY(role(0, Roles::ErrorRole).toString().isEmpty());
        QVERIFY(mModel->isActive());
        QCOMPARE(mModel->finishedCount(), 0);
    }

    void unknownOrRepeatedIdIsSkipped() {
        mStore->add(modWithUpdate(u"carryon"_s));

        mStore->requestUpdate({u"carryon"_s, u"ghost"_s, u"carryon"_s});

        QCOMPARE(ids(), QStringList{u"carryon"_s});
    }

    // the store replaces the entry once installed, the row keeps what it was updated from
    void versionsOutliveTheReplacedEntry() {
        requestTwo();

        mStore->add(vsmm::ModEntry{localInfo(u"carryon"_s, u"1.1.0"_s, QFileInfo{})});

        QCOMPARE(role(0, Roles::FromVersionRole).toString(), u"1.0.0"_s);
        QCOMPARE(role(0, Roles::ToVersionRole).toString(), u"1.1.0"_s);
    }

    void progressChangesOnlyThatRow() {
        requestTwo();
        QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};
        QSignalSpy progressChanged{mModel.get(), &vsmm::ModUpdateModel::progressChanged};

        mStore->reportProgress(u"betterruins"_s, 25, 100);

        QCOMPARE(changed.count(), 1);
        Change first = change(changed.at(0));
        QCOMPARE(first.mFirst, 1);
        QCOMPARE(first.mLast, 1);
        // the first report also starts the download
        QCOMPARE(first.mRoles, (QList<int>{Roles::StateRole, Roles::ProgressRole}));
        QCOMPARE(state(1), State::Downloading);
        QCOMPARE(progress(1), 0.25);
        QCOMPARE(state(0), State::Queued);

        mStore->reportProgress(u"betterruins"_s, 50, 100);

        QCOMPARE(changed.count(), 2);
        QCOMPARE(change(changed.at(1)).mRoles, QList<int>{Roles::ProgressRole});
        QCOMPARE(progress(1), 0.5);
        QCOMPARE(progressChanged.count(), 2);
        QCOMPARE(mModel->totalProgress(), 0.25);
    }

    // no Content-Length, QML shows a busy bar for it
    void unknownTotalGivesIndeterminateProgress() {
        requestTwo();

        mStore->reportProgress(u"carryon"_s, 4096, -1);

        QCOMPARE(state(0), State::Downloading);
        QCOMPARE(progress(0), -1.0);
        QCOMPARE(mModel->totalProgress(), 0.0);
    }

    void resultsMoveRowsToTheirFinalState() {
        requestTwo();
        mStore->reportProgress(u"carryon"_s, 100, 100);
        mStore->reportProgress(u"betterruins"_s, 30, 100);
        QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        mStore->reportInstalling(u"carryon"_s);

        QCOMPARE(state(0), State::Installing);
        QCOMPARE(progress(0), 1.0);
        QCOMPARE(change(changed.at(0)).mRoles, (QList<int>{Roles::StateRole, Roles::ProgressRole, Roles::ErrorRole}));

        mStore->reportFinished(u"carryon"_s);
        mStore->reportFailed(u"betterruins"_s, u"timeout"_s);

        QCOMPARE(state(0), State::Done);
        QCOMPARE(state(1), State::Failed);
        QCOMPARE(role(1, Roles::ErrorRole).toString(), u"timeout"_s);
        // a failed download keeps how far it got
        QCOMPARE(progress(1), 0.3);
        QCOMPARE(mModel->finishedCount(), 2);
        QCOMPARE(mModel->failedCount(), 1);
        QCOMPARE(mModel->totalProgress(), 1.0);
        QVERIFY(!mModel->isActive());
    }

    void reportsAfterTheResultAreIgnored() {
        requestTwo();
        mStore->reportFinished(u"carryon"_s);
        mStore->reportInstalling(u"betterruins"_s);
        QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        mStore->reportFailed(u"carryon"_s, u"late"_s);
        mStore->reportProgress(u"carryon"_s, 1, 2);
        // the download is over once installing starts
        mStore->reportProgress(u"betterruins"_s, 1, 2);

        QCOMPARE(changed.count(), 0);
        QCOMPARE(state(0), State::Done);
        QCOMPARE(state(1), State::Installing);
    }

    // a GUI install of a known mod reports through the same signals
    void reportsForModsOutsideTheBatchAreIgnored() {
        requestTwo();
        QSignalSpy changed{mModel.get(), &QAbstractItemModel::dataChanged};

        mStore->reportProgress(u"ghost"_s, 1, 2);
        mStore->reportInstalling(u"ghost"_s);
        mStore->reportFinished(u"ghost"_s);
        mStore->reportFailed(u"ghost"_s, u"timeout"_s);

        QCOMPARE(changed.count(), 0);
    }

    void newBatchReplacesThePreviousRows() {
        requestTwo();
        mStore->reportFinished(u"carryon"_s);
        mStore->reportFinished(u"betterruins"_s);
        mStore->add(modWithUpdate(u"bees"_s));

        mStore->requestUpdate({u"bees"_s});

        QCOMPARE(ids(), QStringList{u"bees"_s});
        QCOMPARE(state(0), State::Queued);
        QCOMPARE(mModel->finishedCount(), 0);
    }

    void clearDropsAFinishedBatch() {
        requestTwo();
        mStore->reportFinished(u"carryon"_s);
        mStore->reportFailed(u"betterruins"_s, u"timeout"_s);
        QSignalSpy countChanged{mModel.get(), &vsmm::ModUpdateModel::countChanged};

        mModel->clear();

        QCOMPARE(mModel->count(), 0);
        QCOMPARE(countChanged.count(), 1);

        // nothing left, nothing announced
        mModel->clear();
        QCOMPARE(countChanged.count(), 1);
    }

    void clearDoesNothingWhileTheBatchRuns() {
        requestTwo();
        mStore->reportFinished(u"carryon"_s);
        QTest::ignoreMessage(QtWarningMsg, "Cannot clear a running update batch");

        mModel->clear();

        QCOMPARE(mModel->count(), 2);
    }

    // safety net, a row that never got a result fails once loading ends
    void rowWithoutAResultFailsWhenLoadingEnds() {
        requestTwo();
        mStore->reportFinished(u"carryon"_s);
        QTest::ignoreMessage(QtWarningMsg, "Update of betterruins ended without a result");

        mStore->finishLoading();

        QCOMPARE(state(0), State::Done);
        QCOMPARE(state(1), State::Failed);
        QCOMPARE(role(1, Roles::ErrorRole).toString(), u"Update ended without a result"_s);
        // nothing was downloaded
        QCOMPARE(progress(1), 0.0);
        QVERIFY(!mModel->isActive());
    }
};

QTEST_GUILESS_MAIN(ModUpdateModelUnitTest)
#include "ModUpdateModelUnitTest.moc"
