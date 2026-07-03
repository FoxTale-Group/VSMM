import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm

TabBar {
    Layout.fillWidth: true
    spacing: 5

    background: Rectangle {
        color: Theme.colors.windowBackground

        // Optional: A subtle separator line under the whole tab bar
        Rectangle {
            width: parent.width
            height: 2
            anchors.bottom: parent.bottom
            color: Theme.colors.tabPanelBorder
        }
    }
}