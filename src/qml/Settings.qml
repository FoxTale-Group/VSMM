import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm
import VSMMStyle

VsmmWindow {
    id: settingsWindow

    width: 800; height: 600; minimumWidth: 600; minimumHeight: 400;

    windowTitle: qsTr("Settings"); windowIcon: Theme.icons.settingsIcon;

    dialog: true; movable: false; resizable: false;
    modality: Qt.ApplicationModal

    // True while any tab holds a value that differs from Config.
    readonly property bool settingsChanged: generalTab.dirty || configurationTab.dirty || appearanceTab.dirty

    ColumnLayout {
        anchors.fill: parent; anchors.leftMargin: 20; anchors.rightMargin: 20; anchors.topMargin: 5; anchors.bottomMargin: 10;
        spacing: 0

        TabBar {
            id: settingsTabBar
            Layout.fillWidth: true

            TabButton { text: qsTr("General") }
            TabButton { text: qsTr("Configuration") }
            TabButton { text: qsTr("Appearance") }
            //TabButton { text: qsTr("Advanced") }
        }

        StackLayout {
            id: settingsLayout

            Layout.fillWidth: true; Layout.fillHeight: true

            currentIndex: settingsTabBar.currentIndex

            SettingsTab_General{ id: generalTab }
            SettingsTab_Configuration{ id: configurationTab }
            SettingsTab_Appearance{ id: appearanceTab }
            //SettingsTab_Advanced{ id: advancedTab}
        }

        // Settings window buttons
        RowLayout {
            Layout.fillWidth: true;
            Layout.margins: 5
            spacing: 5

            LayoutHorizontalSpacer{}

            Button {
                text: qsTr("Save")

                enabled: settingsChanged

                defaultColor: Theme.colors.accent
                display: AbstractButton.TextOnly
                Layout.preferredHeight: 30

                onClicked: {
                    settingsWindow.saveToConfig()
                    console.log("Settings applied")
                }
            }

            Button {
                text: qsTr("Save && Close")

                enabled: settingsChanged

                defaultColor: Theme.colors.accent
                display: AbstractButton.TextOnly
                Layout.preferredHeight: 30

                onClicked: {
                    settingsWindow.saveToConfig()
                    console.log("Settings applied")
                    settingsWindow.close()
                }
            }

            Button {
                text: qsTr("Close")

                display: AbstractButton.TextOnly
                Layout.preferredHeight: 30

                onClicked: {
                    settingsWindow.resetToCurrentConfig()
                    settingsWindow.close()
                }
            }

            LayoutHorizontalSpacer{}
        }
    }

    Component.onCompleted: resetToCurrentConfig()

    // Restores every tab's controls to the persisted values, discarding edits.
    function resetToCurrentConfig() {
        generalTab.revert();
        configurationTab.revert();
        appearanceTab.revert();
    }

    function saveToConfig() {
        if(Config) {
            let generalCfg = Config.general;
            generalCfg.deleteOldModVersion = generalTab.deleteOldModVersion;
            generalCfg.includeModPrerelease = generalTab.includeModPrerelease;
            Config.general = generalCfg;

            // An empty path is a valid value and is persisted as such.
            let pathsCfg = Config.paths;
            pathsCfg.gameConfig = configurationTab.gameConfigDir;
            pathsCfg.gameExe = configurationTab.gameExePath;
            Config.paths = pathsCfg;

            let appearanceCfg = Config.appearance;
            appearanceCfg.accentIndex = appearanceTab.accentIndex;
            Config.appearance = appearanceCfg;
        }
    }
}