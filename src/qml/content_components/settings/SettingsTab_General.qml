import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import vsmm

VsmmTabPanel {
    id: _SettingsTab_General

    property bool deleteOldModVersion: false

    signal settingsEdited(edited: bool)

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
                    if(_SettingsTab_General.deleteOldModVersion !== Config.general.deleteOldModVersion) {
                        _SettingsTab_General.settingsEdited(true)
                    } else {
                        _SettingsTab_General.settingsEdited(false)
                    }
                }
            }

            Label {
                text: qsTr("Remove old mod versions when manually adding newer one")
                font.pixelSize: 12
                font.bold: true
                color: Theme.colors.label
                
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            CheckBox {
                checked: false

                onCheckedChanged: {

                }
            }

            Label {
                text: qsTr("Check pre-release versions of mods")
                font.pixelSize: 12
                font.bold: true
                color: Theme.colors.label

            }
        }

        LayoutVerticalSpacer{}
    }

}