import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm

VsmmWindow {
    id: windowMain
    width: 860
    height: 640
    minimumWidth: 750
    minimumHeight: 450

    windowTitle: Qt.application.name + "  v" + Qt.application.version
    windowIcon: Theme.appLogo

    Settings {id: settingsWindow}
    AddMod {id: addModWindow}

    ColumnLayout
    {
        id: appContent
        anchors.fill: parent;
        anchors.topMargin: 7; anchors.leftMargin: 20; anchors.rightMargin: 20; anchors.bottomMargin: 20
        spacing: 16

        RowLayout
        {
            Layout.fillWidth: true
            spacing: 12

            VsmmButton {
                text: qsTr("Launch Game"); icon.source: Theme.icons.iLaunch; tooltipText: qsTr("Launch Game")

                display: AbstractButton.TextBesideIcon
                Layout.preferredHeight: 60

                defaultColor:   Theme.colors.buttonLaunchDefault
                hoverColor:     Theme.colors.buttonLaunchHover
                pressColor:     Theme.colors.buttonLaunchPress

                onClicked: {
                    console.info("Launching game...")
                    GameMngr.launchGame();
                }
            }

            VsmmButton {
                icon.source: Theme.icons.iSettings; tooltipText: qsTr("Settings")

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
    }
}