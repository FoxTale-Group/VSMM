import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VSMM.Theme
import VSMM.Config
import VSMM.GameMngr
import VSMM.Templates
import VSMM.ModList
import VSMM.ContentComponents
import VSMMStyle

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

    // Theme holds no reference to Config, so the persisted accent has to be pushed into it.
    // Done here rather than relying on the settings window's revert(), which only reaches
    // Theme because that window happens to be constructed eagerly.
    // Theme clamps the index when resolving the colour, so no range check is needed.
    Component.onCompleted: Theme.accentIndex = Config.appearance.accentIndex ?? 0

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
    }
}