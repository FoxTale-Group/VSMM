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
#include <ModSortFilterModel.hpp>
#include <RealComponents.hpp>

#include <QAbstractItemModelTester>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QTest>

#include <memory>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

class ModSortFilterModelIntegrationTest : public QObject {
    Q_OBJECT

    // only the proxy logs, the rest is pinned by the other suites
    static void onlyProxyLogs() {
        QLoggingCategory::setFilterRules(u"*=false\nmodsortfiltermodel=true\nmodsortfiltermodel.debug=false"_s);
    }

    std::unique_ptr<RealComponents> mApp;
    std::unique_ptr<vsmm::ModListModel> mModel;
    std::unique_ptr<vsmm::ModSortFilterModel> mProxy;
    std::unique_ptr<QAbstractItemModelTester> mModelTester;
    std::unique_ptr<QAbstractItemModelTester> mProxyTester;

    [[nodiscard]] vsmm::ModStore &store() const { return mApp->store(); }

    void addMods(const QStringList &ids) const {
        for (const auto &id : ids) {
            store().add(localInfo(id, u"1.0.0"_s, writeStubZip(mApp->modsDir(), u"%1.zip"_s.arg(id))),
                        vsmm::ModStore::ModLoadType::Init);
        }
    }

    // what the view would show, top to bottom
    [[nodiscard]] QStringList names() const {
        QStringList names;
        for (int row = 0; row < mProxy->rowCount(QModelIndex{}); ++row) {
            names.append(mProxy->data(mProxy->index(row, 0), vsmm::ModListModel::NameRole).toString());
        }
        return names;
    }

  private slots:
    void initTestCase() {
        // the store needs a game version, which comes from a fake game exe, a POSIX shell script
        SKIP_WITHOUT_POSIX_SHELL();
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(RealComponents::configDir().contains("qttest"_L1), qPrintable(RealComponents::configDir()));
    }

    void cleanupTestCase() { QStandardPaths::setTestModeEnabled(false); }

    void init() {
        onlyProxyLogs();

        mApp = std::make_unique<RealComponents>();
        QVERIFY(mApp->start());
        // connected the way App does it
        mModel = std::make_unique<vsmm::ModListModel>();
        mModel->setStore(&store());
        mProxy = std::make_unique<vsmm::ModSortFilterModel>();
        mProxy->setSourceModel(mModel.get());
        mModelTester = std::make_unique<QAbstractItemModelTester>(
            mModel.get(), QAbstractItemModelTester::FailureReportingMode::QtTest);
        mProxyTester = std::make_unique<QAbstractItemModelTester>(
            mProxy.get(), QAbstractItemModelTester::FailureReportingMode::QtTest);
    }

    void cleanup() {
        mProxyTester.reset();
        mModelTester.reset();
        mProxy.reset();
        mModel.reset();
        mApp.reset();
    }

    // the model keeps scan order, the proxy is what sorts
    void scannedModsAreSortedByLocalName() {
        addMods({u"zebrautils"_s, u"carryon"_s, u"animalcages"_s});

        QCOMPARE(names(), QStringList({u"animalcages (local)"_s, u"carryon (local)"_s, u"zebrautils (local)"_s}));
    }

    // online info renames a mod after the proxy placed it, the store to model to proxy chain has to move it
    void onlineNameResortsTheRow() {
        addMods({u"ddd"_s, u"eee"_s, u"zzz"_s});
        QCOMPARE(names().last(), u"zzz (local)"_s);

        store().updateOnline(u"zzz"_s, modJson());

        QCOMPARE(names(), QStringList({u"Carry On"_s, u"ddd (local)"_s, u"eee (local)"_s}));
    }

    void filterMatchesTheOnlineName() {
        addMods({u"ddd"_s, u"zzz"_s});
        mProxy->setFilterText(u"carry"_s);
        QVERIFY(names().isEmpty());

        store().updateOnline(u"zzz"_s, modJson());

        QCOMPARE(names(), QStringList{u"Carry On"_s});
    }

    // a scan running behind an already typed search must respect it
    void filterAppliesToModsScannedLater() {
        mProxy->setFilterText(u"ruins"_s);

        addMods({u"moreruins"_s, u"carryon"_s, u"betterruins"_s});

        QCOMPARE(names(), QStringList({u"betterruins (local)"_s, u"moreruins (local)"_s}));
    }

    void removedModLeavesTheProxy() {
        addMods({u"zebrautils"_s, u"carryon"_s, u"animalcages"_s});

        store().remove(u"carryon"_s);

        QCOMPARE(names(), QStringList({u"animalcages (local)"_s, u"zebrautils (local)"_s}));
    }

    // the search field keeps its text across a rescan, so the refilled list must honor it
    void filterSurvivesAReloadAndRescan() {
        mProxy->setFilterText(u"ruins"_s);
        addMods({u"moreruins"_s, u"carryon"_s, u"betterruins"_s});

        store().reload();

        QVERIFY(names().isEmpty());
        addMods({u"moreruins"_s, u"carryon"_s, u"betterruins"_s});
        QCOMPARE(mProxy->getFilterText(), u"ruins"_s);
        QCOMPARE(names(), QStringList({u"betterruins (local)"_s, u"moreruins (local)"_s}));
    }
};

QTEST_GUILESS_MAIN(ModSortFilterModelIntegrationTest)
#include "ModSortFilterModelIntegrationTest.moc"
