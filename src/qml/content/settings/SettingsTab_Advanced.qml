import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import vsmm

VsmmTabPanel {
    id: _SettingsTab_Advanced
    content: Rectangle {
        anchors.fill: parent; anchors.margins: 10; color: "transparent"

        Label {
            text: qsTr("Dangerous System Options Go Here")
            anchors.centerIn: parent
            color: Theme.colors.label
        }
    }
}