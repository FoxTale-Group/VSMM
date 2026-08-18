import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import VSMM.Theme
import VSMM.Templates

TabPanel {
    id: settingsTabAdvanced
    content: Rectangle {
        anchors.fill: parent; anchors.margins: 10; color: "transparent"

        Label {
            text: qsTr("Dangerous System Options Go Here")
            font.pixelSize: Theme.fonts.body
            anchors.centerIn: parent
        }
    }
}