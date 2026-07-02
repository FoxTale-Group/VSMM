import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import vsmm

Rectangle {
    id: _SettingsTab_Advanced
    color: "transparent"

    Label {
        text: qsTr("Dangerous System Options Go Here")
        anchors.centerIn: parent
        color: Theme.colors.label
    }
}