import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: Theme.radius.panel
    color: Theme.colors.modlistBg
    border.color: Theme.colors.modlistBorder
    border.width: 1
    clip: true

    ListView {
        id: modListView
        anchors.fill: parent; anchors.margins: 1; spacing: 0
        clip: true; reuseItems: true; cacheBuffer: 400

        model: ModSortFilterModel

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        delegate: ModEntry {}
    }
}