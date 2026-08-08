import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm
import VSMMStyle

RowLayout
{
    id: actionButtons
    Layout.fillWidth: true
    spacing: 10

    signal openAddModDialog()

    Button {
        text: qsTr("Add mod")
        icon.source: Theme.icons.addBoxIcon

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: qsTr("Add a new mod from file")

        defaultColor: Theme.colors.buttonAddModDefault
        hoverColor: Theme.colors.buttonAddModHover
        pressColor: Theme.colors.buttonAddModPress

        onClicked: {
            console.log("Add mod button clicked.")
            actionButtons.openAddModDialog()
        }
    }

    ModSearchBar{}

    Button {
        icon.source: Theme.icons.filterIcon
        display: AbstractButton.IconOnly
        Layout.preferredHeight: 40

        tooltipText: qsTr("Filter mods")

        onClicked: {}
    }

    Button {
        icon.source: Theme.icons.syncIcon
        display: AbstractButton.IconOnly
        Layout.preferredHeight: 40

        tooltipText: qsTr("Refresh mod list")

        defaultColor: Theme.colors.highlightButtonDefault
        hoverColor: Theme.colors.highlightButtonHover
        pressColor: Theme.colors.highlightButtonPress

        enabled: ModStore.workPending === false
        onClicked: ModStore.reload()
    }

    Button {
        text: qsTr("Update selected")
        icon.source: Theme.icons.downloadIcon

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

    Button {
        text: qsTr("Update all")
        icon.source: Theme.icons.downloadAllIcon

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