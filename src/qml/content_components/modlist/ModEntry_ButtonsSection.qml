import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Effects
import vsmm

RowLayout {
    spacing: 2
    clip: true
    opacity: _ModEntryHoverHandler.hovered ? 1.0 : 0.0
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation {
            duration: 200
            easing.type: Easing.OutCubic
        }
    }

    VsmmModEntryButton {
        icon.source: Theme.icons.iDownloadOne

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: hasUpdate ? qsTr("Download update for '%1'").arg(name) : ""

        defaultColor: "transparent"
        hoverColor: hasUpdate ? Theme.colors.buttonUpdateHover : "transparent"
        pressColor: hasUpdate ? Theme.colors.buttonUpdatePress : "transparent"

        iconColor: hasUpdate ? Theme.colors.buttonUpdateLabel : Theme.colors.labelVersion

        enabled: hasUpdate

        onClicked: {
            console.log("Updating mod" + name)
            ModStore.update(modid)
        }
    }

    VsmmModEntryButton {
        icon.source: Theme.icons.iCheckUpdate

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: qsTr("Check update for '%1'").arg(name)

        onClicked: {
            // TODO: Implement check for update for single mod
            console.log("Checking update for " + name)
        }
    }

    VsmmModEntryButton {
        property bool favorited: false

        icon.source: favorited ? Theme.icons.iFavoriteFilled : Theme.icons.iFavorite
        iconColor: favorited ? Theme.colors.modFavButton : Theme.colors.icon

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: qsTr("Add '%1' to favorites").arg(name)

        onClicked: {
            favorited = !favorited
            console.log("Added '" + name + "' to favorites")
        }
    }

    VsmmModEntryButton {
        icon.source: Theme.icons.iOpenLink

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: qsTr("Open '%1' mod page").arg(name)

        onClicked: {
            console.info("Opening " + name + " modpage: " + url)
            Qt.openUrlExternally(url)
        }
    }

    VsmmModEntryButton {
        icon.source: Theme.icons.iDelete
        iconColor: Theme.colors.modDelButtonIcon

        Layout.leftMargin: 5

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: qsTr("Delete mod '%1'").arg(name)

        hoverColor: Theme.colors.modDelButtonHover
        pressColor: Theme.colors.modDelButtonPress

        onClicked: {
            console.info("Deleting " + name)
            // TODO: Implement mod deleting functionality
        }
    }
}