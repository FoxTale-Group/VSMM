import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmodchecker

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    radius: 12
    color: "#262626"
    border.color: "#3a3a3a"
    border.width: 1
    clip: true

    ListView {
        id: modListView
        anchors.fill: parent
        Layout.margins: 1
        model: ModListModel
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
                color: vbar.pressed ? "#666666" : (vbar.hovered ? "#555555" : "#444444")
            }
        }

        delegate: ModEntry {}
    }
}