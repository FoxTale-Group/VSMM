import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import vsmm

TabPanel {
    id: _SettingsTab_Appearance

    signal settingsEdited(edited: bool)

    content: ColumnLayout {
        anchors.fill: parent
        spacing: 5

        Label {
            text: qsTr("Application Language")
            font.pixelSize: Theme.fonts.body
        }

        Label {
            text: qsTr("Theme")
            font.pixelSize: Theme.fonts.body
        }

        LayoutVerticalSpacer{}
    }
}