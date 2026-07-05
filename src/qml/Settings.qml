import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm
import "js/StringHelpers.js" as StrUtils

VsmmWindow {
    id: _settingsWindow

    width: 800; height: 600; minimumWidth: 600; minimumHeight: 400;

    windowTitle: qsTr("Settings"); windowIcon: Theme.icons.iSettings;

    dialog: true; movable: false; resizable: false;
    modality: Qt.ApplicationModal

    property bool settingsChanged: false

    ColumnLayout {
        anchors.fill: parent; anchors.leftMargin: 20; anchors.rightMargin: 20; anchors.topMargin: 5; anchors.bottomMargin: 10;
        spacing: 0

        VsmmTabBar {
            id: settingsTabBar

            VsmmTabButton { text: qsTr("General")}
            VsmmTabButton { text: qsTr("Paths")}
            VsmmTabButton { text: qsTr("Appearance")}
            //VsmmTabButton { text: qsTr("Advanced")}
        }

        StackLayout {
            id: settingsLayout

            Layout.fillWidth: true; Layout.fillHeight: true

            currentIndex: settingsTabBar.currentIndex

            SettingsTab_General{ id: generalTab; onSettingsEdited: (edited) => _settingsWindow.settingsChanged = edited}
            SettingsTab_Paths{ id: pathsTab; onSettingsEdited: (edited) => _settingsWindow.settingsChanged = edited}
            SettingsTab_Appearance{ id: appearanceTab; onSettingsEdited: (edited) => _settingsWindow.settingsChanged = edited}
            //SettingsTab_Advanced{ id: advancedTab}
        }

        // Settings window buttons
        RowLayout {
            Layout.fillWidth: true;
            Layout.margins: 5
            spacing: 5

            LayoutHorizontalSpacer{}

            VsmmButton {
                text: settingsChanged ? "* " + qsTr("Save") : qsTr("Save")

                defaultColor: settingsChanged ? Theme.colors.buttonDefault : Theme.colors.buttonInactive
                enabled: settingsChanged

                display: AbstractButton.TextOnly
                Layout.preferredHeight: 30

                onClicked: {
                    _settingsWindow.saveToConfig()
                    settingsChanged = false
                    console.log("Settings applied")
                }
            }

            VsmmButton {
                text: qsTr("Save & Close")

                defaultColor: settingsChanged ? Theme.colors.buttonDefault : Theme.colors.buttonInactive
                enabled: settingsChanged

                display: AbstractButton.TextOnly
                Layout.preferredHeight: 30

                onClicked: {
                    _settingsWindow.saveToConfig()
                    console.log("Settings applied")
                    settingsChanged = false
                    _settingsWindow.close()
                }
            }

            VsmmButton {
                text: qsTr("Close")

                display: AbstractButton.TextOnly
                Layout.preferredHeight: 30

                onClicked: {
                    _settingsWindow.resetToCurrentConfig()
                    _settingsWindow.close()
                }
            }

            LayoutHorizontalSpacer{}
        }
    }

    Component.onCompleted: resetToCurrentConfig()

    function resetToCurrentConfig() {
        if(Config && Config.config && Config.config.vsmm) {
            generalTab.deleteOldModVersion = Config.config.vsmm.deleteOldModVersion
            pathsTab.gameConfigDir = Config.config.vsmm.configGamePath
            pathsTab.gameExePath = Config.config.vsmm.gameExe
        }
        settingsChanged = false
    }

    function saveToConfig() {
        if(Config && Config.config && Config.config.vsmm) {
            let cfg = Config.config;

            cfg.vsmm.deleteOldModVersion = generalTab.deleteOldModVersion

            if(!StrUtils.isNullOrWhitespace(pathsTab.gameConfigDir)) {
                cfg.vsmm.configGamePath = pathsTab.gameConfigDir
            }

            if(!StrUtils.isNullOrWhitespace(pathsTab.gameExePath)) {
                cfg.vsmm.gameExe = pathsTab.gameExePath
            }

            Config.config = cfg;
        }
    }
}