import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm

RowLayout
{
    id: actionButtons
    Layout.fillWidth: true
    spacing: 10

    signal openAddModDialog()

    VsmmButton {
        text: "Add mod"
        icon.source: Theme.icons.iAddBox

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: "Add a new mod from file"

        defaultColor: Theme.colors.buttonAddModDefault
        hoverColor: Theme.colors.buttonAddModHover
        pressColor: Theme.colors.buttonAddModPress

        onClicked: {
            console.log("Add mod button clicked.")
            openAddModDialog()
        }
    }

    ModSearchBar{}

    VsmmButton {
        icon.source: Theme.icons.iFilter
        display: AbstractButton.IconOnly
        Layout.preferredHeight: 40

        tooltipText: "Filter mods"

        onClicked: {}
    }

    VsmmButton {
        icon.source: Theme.icons.iSync
        display: AbstractButton.IconOnly
        Layout.preferredHeight: 40

        tooltipText: "Refresh mod list"

        defaultColor: Theme.colors.highlightButtonDefault
        hoverColor: Theme.colors.highlightButtonHover
        pressColor: Theme.colors.highlightButtonPress

        enabled: ModStore.reloading === false
        onClicked: ModStore.reload()
    }

    VsmmButton {
        text: "Update selected"
        icon.source: Theme.icons.iDownload

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: "Update selected mods"

        defaultColor: Theme.colors.highlightButtonDefault
        hoverColor: Theme.colors.highlightButtonHover
        pressColor: Theme.colors.highlightButtonPress

        onClicked: console.log("Updating selected mods...")
    }

    VsmmButton {
        text: "Update all"
        icon.source: Theme.icons.iDownloadAll

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40
        palette.buttonText: Theme.colors.buttonUpdateLabel

        tooltipText: "Update all mods"

        defaultColor: Theme.colors.buttonUpdateDefault
        hoverColor: Theme.colors.buttonUpdateHover
        pressColor: Theme.colors.buttonUpdatePress

        onClicked: console.log("Updating all mods...")
    }

}