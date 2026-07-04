import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import vsmm

VsmmTabPanel {
    id: _SettingsTab_General

    property bool deleteOldModVersion: false

    content: ColumnLayout {
        anchors.fill: parent
        spacing: 5

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            CheckBox {
                id: deleteModCheck
                checked: _SettingsTab_General.deleteOldModVersion

                onCheckedChanged: {
                    _SettingsTab_General.deleteOldModVersion = checked
                    console.log("deleteOldModVersions: " + _SettingsTab_General.deleteOldModVersion)
                }
            }

            Label {
                text: qsTr("Remove old mod versions when manually adding newer one")
                font.pixelSize: 12
                font.bold: true
                color: Theme.colors.label
                
            }
        }

        LayoutVerticalSpacer{}
    }

}