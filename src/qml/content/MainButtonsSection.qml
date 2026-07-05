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
        text: qsTr("Add mod")
        icon.source: Theme.icons.iAddBox

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: qsTr("Add a new mod from file")

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

        tooltipText: qsTr("Filter mods")

        onClicked: {}
    }

    VsmmButton {
        icon.source: Theme.icons.iSync
        display: AbstractButton.IconOnly
        Layout.preferredHeight: 40

        tooltipText: qsTr("Refresh mod list")

        defaultColor: Theme.colors.highlightButtonDefault
        hoverColor: Theme.colors.highlightButtonHover
        pressColor: Theme.colors.highlightButtonPress

        enabled: ModStore.workPending === false
        onClicked: ModStore.reload()
    }

    VsmmButton {
        text: qsTr("Update selected")
        icon.source: Theme.icons.iDownload

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: qsTr("Update selected mods")

        defaultColor: Theme.colors.highlightButtonDefault
        hoverColor: Theme.colors.highlightButtonHover
        pressColor: Theme.colors.highlightButtonPress

        onClicked: {
            console.log("Updating selected mods...")
            ModStore.updateSelected()
        }
    }

    VsmmButton {
        text: qsTr("Update all")
        icon.source: Theme.icons.iDownloadAll

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40
        palette.buttonText: Theme.colors.buttonUpdateLabel

        tooltipText: qsTr("Update all mods")

        defaultColor: Theme.colors.buttonUpdateDefault
        hoverColor: Theme.colors.buttonUpdateHover
        pressColor: Theme.colors.buttonUpdatePress

        onClicked: {
            console.log("Updating all mods...")
            ModStore.updateAll()
        }
    }

}