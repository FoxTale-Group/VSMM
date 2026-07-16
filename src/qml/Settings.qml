import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm
import VSMMStyle
import "js/StringHelpers.js" as StrUtils

VsmmWindow {
    id: _settingsWindow

    width: 800; height: 600; minimumWidth: 600; minimumHeight: 400;

    windowTitle: qsTr("Settings"); windowIcon: Theme.icons.settingsIcon;

    dialog: true; movable: false; resizable: false;
    modality: Qt.ApplicationModal

    property bool settingsChanged: false

    ColumnLayout {
        anchors.fill: parent; anchors.leftMargin: 20; anchors.rightMargin: 20; anchors.topMargin: 5; anchors.bottomMargin: 10;
        spacing: 0

        TabBar {
            id: settingsTabBar
            Layout.fillWidth: true

            TabButton { text: qsTr("General"); font.pixelSize: Theme.fonts.body }
            TabButton { text: qsTr("Paths"); font.pixelSize: Theme.fonts.body }
            TabButton { text: qsTr("Appearance"); font.pixelSize: Theme.fonts.body }
            //TabButton { text: qsTr("Advanced")}
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

            Button {
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

            Button {
                text: qsTr("Save && Close")

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

            Button {
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
        if(Config) {
            generalTab.deleteOldModVersion = Config.general.deleteOldModVersion ?? true
            generalTab.includeModPrerelease = Config.general.includeModPrerelease ?? false
            pathsTab.gameConfigDir = Config.paths.gameConfig ?? ""
            pathsTab.gameExePath = Config.paths.gameExe ?? ""
        }
        settingsChanged = false
    }

    function saveToConfig() {
        if(Config) {
            let generalCfg = Config.general;
            generalCfg.deleteOldModVersion = generalTab.deleteOldModVersion;
            generalCfg.includeModPrerelease = generalTab.includeModPrerelease;
            Config.general = generalCfg;

            let pathsCfg = Config.paths;
            if(!StrUtils.isNullOrWhitespace(pathsTab.gameConfigDir)) {
                pathsCfg.gameConfig = pathsTab.gameConfigDir
            }

            if(!StrUtils.isNullOrWhitespace(pathsTab.gameExePath)) {
                pathsCfg.gameExe = pathsTab.gameExePath
            }

            Config.paths = pathsCfg;
        }
    }
}