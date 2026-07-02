import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: 12
    color: Theme.colors.modlistBg
    border.color: Theme.colors.modlistBorder
    border.width: 1
    clip: true

    ListView {
        id: modListView
        anchors.fill: parent
        Layout.margins: 1
        model: ModSortFilterModel
        spacing: 0
        clip: true

        ScrollBar.vertical: ScrollBar {
            id: vbar
            policy: ScrollBar.AlwaysOn
            width: 16
            background: Item {}

            // Handle
            contentItem: Rectangle {
                implicitWidth: 16
                radius: 8
                color: vbar.pressed ? Theme.colors.scrollBarHandlePress : (vbar.hovered ? Theme.colors.scrollBarHandleHover : Theme.colors.scrollBarHandleDefault)
            }
        }

        delegate: ModEntry {}
    }
}