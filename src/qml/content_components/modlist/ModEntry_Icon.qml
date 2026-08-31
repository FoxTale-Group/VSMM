import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Effects
import VSMM.Theme

Rectangle {
    id: modIcon
    Layout.topMargin: 12
    Layout.bottomMargin: 12
    Layout.leftMargin: 0
    Layout.rightMargin: 0
    implicitWidth: 40
    implicitHeight: 40
    radius: Theme.radius.card
    color: Theme.colors.modIconBg

    property string coverUrl: modThumbnail

    // Fallback icon with background
    Rectangle {
        id: background
        anchors.fill: parent
        radius: modIcon.radius
        color: Theme.colors.modIconBgDefault
        visible: mainImage.status !== Image.Ready

        IconImage {
            id: fallbackIcon
            source: Theme.icons.extensionIcon
            color: Theme.colors.icon
            anchors.fill: parent
            anchors.margins: 4
            sourceSize.width: modIcon.width
            sourceSize.height: modIcon.height
            visible: mainImage.status !== Image.Ready
        }
    }

    // Rounded square mask shape
    Rectangle {
        id: maskTemplate
        anchors.fill: parent
        radius: modIcon.radius
        visible: false
        layer.enabled: true
    }

    // Mod icon from icon provider, masked through its own layer effect
    Image {
        id: mainImage
        source: modIcon.coverUrl
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        sourceSize.width: modIcon.width
        sourceSize.height: modIcon.height
        visible: mainImage.status === Image.Ready

        // layer.effect, not a sibling MultiEffect: the item stays visible, so its node keeps
        // updating and a late-arriving image is not stuck at the texture captured on realization
        layer.enabled: mainImage.status === Image.Ready
        layer.effect: MultiEffect {
            maskEnabled: true
            maskSource: maskTemplate
        }
    }

    // Icon image border
    Rectangle {
        color: "transparent"
        anchors.fill: parent
        implicitWidth: parent.width
        implicitHeight: parent.height
        radius: parent.radius * 625E-3 // Convert border radius to 62.5% of background radius
        border.width: 1
        border.color: Theme.colors.modIconBorder
    }
}
