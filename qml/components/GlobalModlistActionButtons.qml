import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmodchecker

RowLayout
{
    id: actionButtons
    Layout.fillWidth: true
    spacing: 16

    ActionButton {
        text: "Add mod"
        icon.source: "qrc:/qt/qml/vsmodchecker/icons/add_box.svg"

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: "Add a new mod from file"

        defaultColor: "#33914e"
        hoverColor: "#2d7140"
        pressColor: "#234e19"

        onClicked: console.log("Open add mod dialog")
    }

    Item { Layout.fillWidth: true } // Spring

    ActionButton {
        icon.source: "qrc:/qt/qml/vsmodchecker/icons/sync.svg"
        display: AbstractButton.IconOnly
        Layout.preferredHeight: 40

        tooltipText: "Refresh mod list"



        onClicked: ModManager.reloadMods()
    }

    ActionButton {
        text: "Check updates"
        icon.source: "qrc:/qt/qml/vsmodchecker/icons/check_update.svg"

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: "Check for mod updates"

        defaultColor: "#237cb8"
        hoverColor: "#1f5c87"
        pressColor: "#143f5c"

        onClicked: console.log("Checking for updates...")
    }

    ActionButton {
        text: "Update selected"
        icon.source: "qrc:/qt/qml/vsmodchecker/icons/download.svg"

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: "Update selected mods"

        defaultColor: "#237cb8"
        hoverColor: "#1f5c87"
        pressColor: "#143f5c"

        onClicked: console.log("Updating selected mods...")
    }

    ActionButton {
        text: "Update all"
        icon.source: "qrc:/qt/qml/vsmodchecker/icons/download_all.svg"

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40
        palette.buttonText: "#e0a23a"

        tooltipText: "Update selected mods"

        defaultColor: "#a6842e"
        hoverColor: "#7c6220"
        pressColor: "#453713"

        onClicked: console.log("Updating all mods...")
    }

}