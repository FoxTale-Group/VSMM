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

#include "ModSortFilterModelTestUtils.hpp"

#include <ModSortFilterModel.hpp>

#include <QLoggingCategory>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace vsmm::test;

namespace {
// four mods whose zip order is neither alphabetical nor case-consistent
[[nodiscard]] QStringList unsortedNames() {
    return {u"zebra Utils"_s, u"Carry On"_s, u"animal Cages"_s, u"Better Ruins"_s};
}
} // namespace

class ModSortFilterModelUnitTest : public QObject {
    Q_OBJECT

    // only this component logs, and no tracing unless a test asks for it
    static void onlyProxyLogs() {
        QLoggingCategory::setFilterRules(u"*=false\nmodsortfiltermodel=true\nmodsortfiltermodel.debug=false"_s);
    }
    static void enableProxyTracing() {
        QLoggingCategory::setFilterRules(u"*=false\nmodsortfiltermodel=true\nmodsortfiltermodel.debug=true"_s);
    }

  private slots:
    void init() { onlyProxyLogs(); }

    void freshProxyIsEmptyAndUnfiltered() {
        const vsmm::ModSortFilterModel proxy;

        QVERIFY(proxy.getFilterText().isEmpty());
        QCOMPARE(proxy.rowCount(QModelIndex{}), 0);
    }

    // the filter can be typed before the first scan hands the view a source model
    void filterSetBeforeTheSourceModelSurvives() {
        vsmm::ModSortFilterModel proxy;
        enableProxyTracing();
        // the source row count comes from a null-guarded sourceModel(), so this is the guard's test
        QTest::ignoreMessage(QtDebugMsg, "Filter set to 'carry', 0 of 0 rows visible");

        proxy.setFilterText(u"carry"_s);

        QCOMPARE(proxy.getFilterText(), u"carry"_s);
        QCOMPARE(proxy.rowCount(QModelIndex{}), 0);

        // and it applies to the model attached afterwards
        onlyProxyLogs();
        SourceModelStub source{unsortedNames()};
        proxy.setSourceModel(&source);

        QCOMPARE(visibleNames(proxy), QStringList({u"Carry On"_s}));
    }

    void sortsByNameCaseInsensitively_data() {
        QTest::addColumn<QStringList>("names");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("mixed-case") << unsortedNames()
                                    << QStringList(
                                           {u"animal Cages"_s, u"Better Ruins"_s, u"Carry On"_s, u"zebra Utils"_s});
        QTest::newRow("case-only-difference")
            << QStringList({u"carry on"_s, u"CARRY"_s}) << QStringList({u"CARRY"_s, u"carry on"_s});
        QTest::newRow("already-sorted") << QStringList({u"a"_s, u"b"_s}) << QStringList({u"a"_s, u"b"_s});
        QTest::newRow("single-row") << QStringList({u"Carry On"_s}) << QStringList({u"Carry On"_s});
        QTest::newRow("empty") << QStringList{} << QStringList{};
    }

    // sorting happens when the source is attached, no explicit sort() call from the outside
    void sortsByNameCaseInsensitively() {
        QFETCH(const QStringList, names);
        QFETCH(const QStringList, expected);

        SourceModelStub source{names};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);

        QCOMPARE(visibleNames(proxy), expected);
    }

    // a mod scanned later must not land at the bottom, dynamic sorting is what puts it in place
    void rowAddedLaterLandsSorted() {
        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);

        source.append(u"Bees"_s);

        QCOMPARE(visibleNames(proxy),
                 QStringList({u"animal Cages"_s, u"Bees"_s, u"Better Ruins"_s, u"Carry On"_s, u"zebra Utils"_s}));
    }

    // the online info replaces the zip name, so a row can be renamed after it was placed
    void renamedRowIsResorted() {
        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);

        source.rename(0, u"aaa Utils"_s); // "zebra Utils" moves from last to first

        QCOMPARE(visibleNames(proxy),
                 QStringList({u"aaa Utils"_s, u"animal Cages"_s, u"Better Ruins"_s, u"Carry On"_s}));
    }

    void removedRowLeavesTheProxy() {
        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);

        source.removeAt(1); // "Carry On"

        QCOMPARE(visibleNames(proxy), QStringList({u"animal Cages"_s, u"Better Ruins"_s, u"zebra Utils"_s}));
    }

    void filterMatchesNameSubstringCaseInsensitively_data() {
        QTest::addColumn<QString>("filterText");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("empty-accepts-all")
            << QString{} << QStringList({u"animal Cages"_s, u"Better Ruins"_s, u"Carry On"_s, u"zebra Utils"_s});
        QTest::newRow("prefix") << u"Carry"_s << QStringList({u"Carry On"_s});
        QTest::newRow("lowercase-matches-capital") << u"carry"_s << QStringList({u"Carry On"_s});
        QTest::newRow("uppercase-matches-lowercase") << u"ZEBRA"_s << QStringList({u"zebra Utils"_s});
        QTest::newRow("mid-word") << u"nima"_s << QStringList({u"animal Cages"_s});
        // matches are kept in sorted order, not in match order
        QTest::newRow("several-matches") << u"a"_s << QStringList({u"animal Cages"_s, u"Carry On"_s, u"zebra Utils"_s});
        QTest::newRow("space") << u"l C"_s << QStringList({u"animal Cages"_s});
        QTest::newRow("no-match") << u"nothing"_s << QStringList{};
    }

    void filterMatchesNameSubstringCaseInsensitively() {
        QFETCH(const QString, filterText);
        QFETCH(const QStringList, expected);

        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);

        proxy.setFilterText(filterText);

        QCOMPARE(visibleNames(proxy), expected);
    }

    void clearingTheFilterRestoresEveryRow() {
        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);
        proxy.setFilterText(u"carry"_s);
        QCOMPARE(proxy.rowCount(QModelIndex{}), 1);

        proxy.setFilterText(QString{});

        QCOMPARE(visibleNames(proxy),
                 QStringList({u"animal Cages"_s, u"Better Ruins"_s, u"Carry On"_s, u"zebra Utils"_s}));
    }

    // a scan running behind an already typed search must respect it
    void filterAppliesToRowsAddedAfterwards() {
        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);
        proxy.setFilterText(u"ruins"_s);

        source.append(u"More Ruins"_s);
        source.append(u"Carry On Too"_s);

        QCOMPARE(visibleNames(proxy), QStringList({u"Better Ruins"_s, u"More Ruins"_s}));
    }

    // renaming into and out of the filter has to move the row in and out of the view
    void renamedRowIsRefiltered() {
        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);
        proxy.setFilterText(u"ruins"_s);
        QCOMPARE(visibleNames(proxy), QStringList({u"Better Ruins"_s}));

        source.rename(1, u"Carry On Ruins"_s);
        QCOMPARE(visibleNames(proxy), QStringList({u"Better Ruins"_s, u"Carry On Ruins"_s}));

        source.rename(3, u"Plain Ruins"_s); // "Better Ruins" is source row 3
        QCOMPARE(visibleNames(proxy), QStringList({u"Carry On Ruins"_s, u"Plain Ruins"_s}));
    }

    void filterTextChangedIsEmittedOnlyOnAChange() {
        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);
        const QSignalSpy spy{&proxy, &vsmm::ModSortFilterModel::filterTextChanged};

        proxy.setFilterText(u"carry"_s);
        QCOMPARE(spy.count(), 1);

        proxy.setFilterText(u"carry"_s); // same text, nothing to tell QML about
        QCOMPARE(spy.count(), 1);

        proxy.setFilterText(u"Carry"_s); // case differs, so the stored text differs
        QCOMPARE(spy.count(), 2);

        proxy.setFilterText(QString{});
        QCOMPARE(spy.count(), 3);
    }

    // the search field binds to this property by name, so the metaobject wiring is part of the contract
    void filterTextIsReachableAsAProperty() {
        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);

        QVERIFY(proxy.setProperty("filterText", u"carry"_s));

        QCOMPARE(proxy.property("filterText").toString(), u"carry"_s);
        QCOMPARE(visibleNames(proxy), QStringList({u"Carry On"_s}));
    }

    void tracingReportsVisibleAndTotalRows() {
        SourceModelStub source{unsortedNames()};
        vsmm::ModSortFilterModel proxy;
        proxy.setSourceModel(&source);

        enableProxyTracing();
        QTest::ignoreMessage(QtDebugMsg, "Filter set to 'a', 3 of 4 rows visible");

        proxy.setFilterText(u"a"_s);
    }
};

QTEST_GUILESS_MAIN(ModSortFilterModelUnitTest)
#include "ModSortFilterModelUnitTest.moc"
