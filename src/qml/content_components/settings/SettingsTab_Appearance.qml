import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import vsmm

VsmmTabPanel {
    id: _SettingsTab_Appearance

    signal settingsEdited(edited: bool)

    content: ColumnLayout {
        anchors.fill: parent
        spacing: 5

        Label {
            text: qsTr("Application Language")
            color: Theme.colors.label
        }

        Label {
            text: qsTr("Theme")
            color: Theme.colors.label
        }

        LayoutVerticalSpacer{}
    }
}