import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VSMM.Theme
import VSMM.ModStore
import VSMMStyle

RowLayout
{
    id: actionButtons
    Layout.fillWidth: true
    spacing: 10

    signal openAddModDialog()

    ModSearchBar{}

    Button {
        icon.source: Theme.icons.filterIcon
        display: AbstractButton.IconOnly
        Layout.preferredHeight: 40

        tooltipText: qsTr("Filter mods")

        onClicked: {}
    }

    Button {
        text: qsTr("Sync")
        icon.source: Theme.icons.syncIcon
        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: qsTr("Refresh mod list")

        defaultColor: Theme.colors.accent

        enabled: ModStore.workPending === false
        onClicked: ModStore.reload()
    }

    Button {
        id: updateButton

        text: qsTr("Update all")
        icon.source: Theme.icons.downloadAllIcon
        tooltipText: qsTr("Update all mods")

        defaultColor: Theme.colors.accent

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        // Nothing to do while a sync runs, or when there is neither a selection nor any available update.
        enabled: !ModStore.workPending && (ModStore.modsSelected || ModStore.updatesCount > 0)

        states: State {
            name: "hasSelection"
            when: ModStore.modsSelected

            PropertyChanges {
                updateButton.text: qsTr("Update selected")
                updateButton.icon.source: Theme.icons.downloadIcon
                updateButton.tooltipText: qsTr("Update selected mods")
            }
        }

        onClicked: {
            if(ModStore.modsSelected) {
                console.log("Updating selected mods...")
                ModStore.updateSelected()
            } else {
                console.log("Updating all mods...")
                ModStore.updateAll()
            }
        }
    }

    Button {
        text: qsTr("Install mod")
        icon.source: Theme.icons.addBoxIcon

        display: AbstractButton.TextBesideIcon
        Layout.preferredHeight: 40

        tooltipText: qsTr("Add a new mod from file")

        defaultColor: Theme.colors.accent

        onClicked: {
            console.log("Add mod button clicked.")
            actionButtons.openAddModDialog()
        }
    }
}