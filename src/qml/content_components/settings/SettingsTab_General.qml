import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import vsmm

TabPanel {
    id: _SettingsTab_General

    property bool deleteOldModVersion: false
    property bool includeModPrerelease: false

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
                font.pixelSize: Theme.fonts.body

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
                font.pixelSize: Theme.fonts.body
                font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            CheckBox {
                font.pixelSize: Theme.fonts.body
                checked: _SettingsTab_General.includeModPrerelease

                onCheckedChanged: {
                    _SettingsTab_General.includeModPrerelease = checked
                    console.log("includeModPrerelease: " + _SettingsTab_General.includeModPrerelease)
                    if(_SettingsTab_General.includeModPrerelease !== Config.general.includeModPrerelease) {
                        _SettingsTab_General.settingsEdited(true)
                    } else {
                        _SettingsTab_General.settingsEdited(false)
                    }
                }
            }

            Label {
                text: qsTr("Check pre-release versions of mods")
                font.pixelSize: Theme.fonts.body
                font.bold: true
            }
        }

        LayoutVerticalSpacer{}
    }

}