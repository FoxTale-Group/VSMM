import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm

RowLayout
{
    id: actionButtons
    Layout.fillWidth: true
    spacing: 16

    signal openAddModDialog()

    ActionButton {
        text: "Add mod"
        buttonIcon: "add_box.svg"

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: "Add a new mod from file"

        defaultColor: "#1D9E75"
        hoverColor: "#188160"
        pressColor: "#0d5e44"

        onClicked: {
            console.log("Add mod button clicked.")
            openAddModDialog()
        }
    }

    //Item { Layout.fillWidth: true } // Spring

    ModSearchBar{}

    ActionButton {
        buttonIcon: "sync.svg"
        display: AbstractButton.IconOnly
        Layout.preferredHeight: 40

        tooltipText: "Refresh mod list"
        enabled: ModStore.reloading === false
        onClicked: ModStore.reload()
    }

    ActionButton {
        text: "Check updates"
        buttonIcon: "check_update.svg"

        display: AbstractButton.IconOnly
        Layout.preferredHeight: 40

        tooltipText: "Check for mod updates"

        defaultColor: "#237cb8"
        hoverColor: "#1f5c87"
        pressColor: "#143f5c"

        onClicked: console.log("Checking for updates...")
    }

    ActionButton {
        text: "Update selected"
        buttonIcon: "download.svg"

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
        buttonIcon: "download_all.svg"

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