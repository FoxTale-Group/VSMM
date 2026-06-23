import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmodchecker

ColumnLayout
{
    id: appContent
    anchors.fill: parent
    anchors.topMargin: 7
    anchors.leftMargin: 20
    anchors.rightMargin: 20
    anchors.bottomMargin: 20
    spacing: 16

    signal openSettingsClicked()

    signal relayOpenAddModDialog()

    RowLayout
    {
        Layout.fillWidth: true
        spacing: 12

        ActionButton {
            text: "Launch Game"
            icon.source: "qrc:/qt/qml/vsmodchecker/icons/launch.svg"

            display: AbstractButton.TextBesideIcon
            Layout.preferredHeight: 60

            tooltipText: "Launch Game"

            defaultColor: "#33914e"
            hoverColor: "#2a7840"
            pressColor: "#1e572e"

            onClicked: {console.log("Launching game...")}
        }

        ActionButton {
            icon.source: "qrc:/qt/qml/vsmodchecker/icons/settings.svg"

            display: AbstractButton.IconOnly

            tooltipText: "App settings"

            onClicked: {
                console.log("Open Settings clicked")
                openSettingsClicked()
            }
        }

        Item { Layout.fillWidth: true }

        ModsStatCards{}
    }

    GlobalModlistActionButtons {
        onOpenAddModDialog: {
            relayOpenAddModDialog()
        }
    }
    ModSearchBar{}
    ModList{}
}