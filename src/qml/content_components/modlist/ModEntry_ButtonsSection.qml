import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Effects
import vsmm
import VSMMStyle

RowLayout {
    spacing: 2
    Layout.rightMargin: 8
    clip: true
    opacity: modEntryHoverHandler.hovered ? 1.0 : 0.0
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation {
            duration: 200
            easing.type: Easing.OutCubic
        }
    }

    Button {
        modEntry: true
        icon.source: Theme.icons.downloadOneIcon

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: modHasUpdate ? qsTr("Download update for '%1'").arg(modName) : ""

        // Shaded from the update role's base rather than from `defaultColor`, which is
        // transparent here and so has nothing to derive from.
        defaultColor: "transparent"
        hoverColor: modHasUpdate ? Theme.shade(Theme.colors.buttonUpdate, Theme.hoverShade) : "transparent"
        pressColor: modHasUpdate ? Theme.shade(Theme.colors.buttonUpdate, Theme.pressShade) : "transparent"

        iconColor: modHasUpdate ? Theme.colors.buttonUpdateLabel : Theme.colors.labelVersion

        enabled: modHasUpdate

        onClicked: {
            console.log("Updating mod" + modName)
            ModStore.update(modId)
        }
    }


    Button {
        modEntry: true

        icon.source: isFavoriteMod ? Theme.icons.favoriteFilledIcon : Theme.icons.favoriteIcon
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

    Button {
        modEntry: true

        icon.source: Theme.icons.openLinkIcon

        Layout.preferredHeight: 35
        Layout.preferredWidth: 35

        tooltipText: qsTr("Open '%1' mod page").arg(modName)

        onClicked: {
            console.info("Opening " + modName + " modpage: " + modUrl)
            Qt.openUrlExternally(modUrl)
        }
    }

    Button {
        modEntry: true

        icon.source: Theme.icons.deleteIcon
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