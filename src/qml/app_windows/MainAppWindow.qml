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
    windowIcon: "logo/VSMM.png"

    SettingsWindow {
        id: settingsWindow
    }

    AddModWindow {
        id: addModWindow
    }

    ColumnLayout
    {
        id: appContent
        anchors.fill: parent
        anchors.topMargin: 7
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        spacing: 16

        RowLayout
        {
            Layout.fillWidth: true
            spacing: 12

            ActionButton {
                text: "Launch Game"
                icon.source: "qrc:/qt/qml/vsmm/assets/icons/launch.svg"

                display: AbstractButton.TextBesideIcon
                Layout.preferredHeight: 60

                tooltipText: "Launch Game"

                defaultColor: "#33914e"
                hoverColor: "#2a7840"
                pressColor: "#1e572e"

                onClicked: {console.log("Launching game...")}
            }

            ActionButton {
                icon.source: "qrc:/qt/qml/vsmm/assets/icons/settings.svg"

                display: AbstractButton.IconOnly

                tooltipText: "Settings"

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

        ModlistActionButtons {
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