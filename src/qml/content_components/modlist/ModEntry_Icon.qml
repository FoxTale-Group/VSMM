import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Effects
import vsmm

// The mod icon doubles as the CheckBox: the icon face sits under the indicator, which is
// fully transparent while unchecked. Checked, the icon blurs behind a translucent fill and
// the style's tick. Hover is shown on the icon's own border, since the outline is switched off.
CheckBox {
    id: modIcon
    Layout.topMargin: 12
    Layout.bottomMargin: 12
    Layout.leftMargin: 0
    Layout.rightMargin: 0
    padding: 0
    boxSize: 40
    boxRadius: Theme.radius.card
    showOutline: false
    checkedColor: Qt.rgba(Theme.colors.modIconCheckedFill.r, Theme.colors.modIconCheckedFill.g,
                      Theme.colors.modIconCheckedFill.b, checkedFillOpacity)

    property string coverUrl: modThumbnail
    property bool modEnabled: true
    property real checkedFillOpacity: 0.4
    // 0 disables the blur; 1 is the strongest
    property real checkedBlur: 1

    Item {
        id: iconFace
        anchors.fill: parent
        z: -1

        Rectangle {
            id: iconBackground
            anchors.fill: parent
            radius: modIcon.boxRadius
            color: Theme.colors.modIconBg
        }

        // Fallback icon with background
        Rectangle {
            id: background
            anchors.fill: parent
            radius: modIcon.boxRadius
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
            radius: modIcon.boxRadius
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
                saturation: modIcon.modEnabled ? 0.0 : -1.0
                blurEnabled: modIcon.checkedBlur > 0
                blur: modIcon.checked ? modIcon.checkedBlur : 0.0
                blurMax: 16
                // Keeps the output the icon's size so the rounded mask still lines up
                autoPaddingEnabled: false

                Behavior on saturation { NumberAnimation { duration: modIcon.morphDuration } }
                Behavior on blur {
                    NumberAnimation {
                        duration: modIcon.morphDuration
                        easing.type: Easing.Bezier
                        easing.bezierCurve: modIcon.easeCurve
                    }
                }
            }
        }

        // Icon image border
        Rectangle {
            color: "transparent"
            anchors.fill: parent
            radius: modIcon.boxRadius * 625E-3 // Convert border radius to 62.5% of background radius
            border.width: 1
            border.color: modIcon.hovered ? Theme.colors.checkBoxBorderHover : Theme.colors.modIconBorder

            Behavior on border.color { ColorAnimation { duration: modIcon.strokeDuration } }
        }
    }
}
