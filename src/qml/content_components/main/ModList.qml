import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm
import VSMM.ModSortFilterModel

Pane {
    Layout.fillWidth: true
    Layout.fillHeight: true
    padding: 1
    clip: true

    ListView {
        id: modListView
        anchors.fill: parent; spacing: 0
        clip: true; reuseItems: true; cacheBuffer: 400

        model: ModSortFilterModel

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        delegate: ModEntry {}
    }
}