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

        tooltipText: modHasUpdate ? qsTr("Download update for '%1'").arg(modName) : ""

        defaultColor: "transparent"
        hoverColor: modHasUpdate ? Theme.colors.buttonUpdateHover : "transparent"
        pressColor: modHasUpdate ? Theme.colors.buttonUpdatePress : "transparent"

        iconColor: modHasUpdate ? Theme.colors.buttonUpdateLabel : Theme.colors.labelVersion

        enabled: modHasUpdate

        onClicked: {
            console.log("Updating mod" + modName)
            ModStore.update(modId)
        }
    }

    VsmmModEntryButton {
        icon.source: isFavoriteMod ? Theme.icons.iFavoriteFilled : Theme.icons.iFavorite
        iconColor: isFavoriteMod ? Theme.colors.modFavButton : Theme.colors.icon

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: !isFavoriteMod ? qsTr("Add '%1' to favorites").arg(modName) : qsTr("Remove '%1' from favorites").arg(modName)

        onClicked: {
            if (!isFavoriteMod) {
                console.info("Added '" + modName + "' to favorites")
            } else {
                console.info("Removed '" + modName + "' from favorites")
            }
            ModStore.setFavorite(modId, !isFavoriteMod);
        }
    }

    VsmmModEntryButton {
        icon.source: Theme.icons.iOpenLink

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: qsTr("Open '%1' mod page").arg(modName)

        onClicked: {
            console.info("Opening " + modName + " modpage: " + modUrl)
            Qt.openUrlExternally(modUrl)
        }
    }

    VsmmModEntryButton {
        icon.source: Theme.icons.iDelete
        iconColor: Theme.colors.modDelButtonIcon

        Layout.leftMargin: 5

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: qsTr("Delete mod '%1'").arg(modName)

        hoverColor: Theme.colors.modDelButtonHover
        pressColor: Theme.colors.modDelButtonPress

        onClicked: {
            console.info("Deleting " + modName)
            ModStore.remove(modId)
        }
    }
}