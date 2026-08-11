import QtQuick
import QtQuick.Templates as T
import vsmm

T.TabButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 6

    property int borderThickness: 2
    property color borderColor: {
        control.checked ? Theme.colors.tabPanelBorder :
            control.hovered ? Theme.colors.tabButtonHoverBorder : Theme.colors.tabButtonInactiveBorder
    }

    property color bottomBorder: {
        control.checked ? Theme.colors.tabPanelBackground : Theme.colors.tabPanelBorder
    }

    property color backgroundColor: {
        control.checked ? Theme.colors.tabPanelBackground :
            control.hovered ? Theme.colors.tabButtonHover : Theme.colors.tabButtonInactive
    }

    property color textColor: {
        control.checked ? Theme.colors.label :
            control.hovered ? Theme.colors.label : Theme.colors.labelAlt
    }

    contentItem: Text {
        text: control.text
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        color: control.textColor
        font.pixelSize: Theme.fonts.label
        font.bold: true
    }

    // Main background rectangle with rounded corners
    background: Rectangle {
        color: control.backgroundColor
        radius: Theme.radius.button
        border.width: control.borderThickness
        border.color: control.borderColor

        // The bottom-half rectangle without rounded corners
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            width: parent.width
            height: parent.height - parent.radius

            color: parent.color

            // The left border for bottom half
            Rectangle {
                width: control.borderThickness
                height: parent.height
                anchors.left: parent.left
                color: control.borderColor
            }
            // The right border for bottom half
            Rectangle {
                width: control.borderThickness
                height: parent.height
                anchors.right: parent.right
                color: control.borderColor
            }
        }

        // The bottom border
        Rectangle {
            width: control.checked ? parent.width - 4 : parent.width
            height: control.borderThickness
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter

            color: control.bottomBorder
        }
    }
}
