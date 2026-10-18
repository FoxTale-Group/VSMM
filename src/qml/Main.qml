import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm
import VSMMStyle

VsmmWindow {
    id: windowMain
    width: 860
    height: 600
    minimumWidth: 800
    minimumHeight: 600

    windowTitle: Qt.application.name + "  v" + Qt.application.version
    windowIcon: Theme.appLogo

    Settings {id: settingsWindow}
    AddMod {id: addModWindow}

    // Theme holds no reference to Config
    // This ensures user-selected accent is loaded from config on startup with main window creation
    // Settings' revert() also does it, but only because that window is created at startup
    Component.onCompleted: Theme.accentIndex = Config.appearance.accentIndex ?? 0

    ColumnLayout
    {
        id: appContent
        anchors.fill: parent;
        anchors.topMargin: 6; anchors.leftMargin: 12; anchors.rightMargin: 12; anchors.bottomMargin: 4
        spacing: 8

        RowLayout
        {
            Layout.fillWidth: true
            spacing: 12

            Button {
                text: qsTr("Launch Game"); icon.source: Theme.icons.launchIcon; tooltipText: qsTr("Launch Game")

                display: AbstractButton.TextBesideIcon
                Layout.preferredHeight: 60
                LayoutMirroring.enabled: true

                defaultColor:   Theme.colors.buttonLaunch

                onClicked: {
                    console.info("Launching game...")
                    GameMngr.launchGame();
                }
            }

            Button {
                icon.source: Theme.icons.settingsIcon; tooltipText: qsTr("Settings")

                display: AbstractButton.IconOnly

                onClicked: {
                    console.log("Opened Settings")
                    settingsWindow.show()
                    settingsWindow.raise()
                    settingsWindow.requestActivate()
                }
            }

            Item { Layout.fillWidth: true }

            StatCards{}
        }

        MainButtonsSection {
            onOpenAddModDialog: {
                console.log("Opened add mod dialog")
                addModWindow.show()
                addModWindow.raise()
                addModWindow.requestActivate()
            }
        }

        ModList{}

        UpdatesPane {id: updatesPanel}

    }

    footer: Item {
        implicitHeight: 30

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12; anchors.rightMargin: 12; anchors.bottomMargin: 12
            spacing: 4

            Button {
                flat: true
                icon.source: Theme.icons.downloadingIcon
                icon.width: 16; icon.height: 16
                tooltipText: qsTr("Show mod update(s) status")
                onClicked: updatesPanel.open ? updatesPanel.hide() : updatesPanel.show()
            }

            LayoutHorizontalSpacer {}

            BusyIndicator {
                running: ModStore.workPending
                Layout.preferredWidth: 16; Layout.preferredHeight: 16
            }
        }
    }
}