import QtQuick
import QtQuick.Controls

TabButton {
    id: _control

    property int borderThickness: 2
    property color borderColor: {
        _control.checked ? Theme.colors.tabPanelBorder :
            _control.hovered ? Theme.colors.tabButtonHoverBorder : Theme.colors.tabButtonInactiveBorder
    }

    property color bottomBorder: {
        _control.checked ? Theme.colors.tabPanelBackground : Theme.colors.tabPanelBorder
    }

    property color backgroundColor: {
        _control.checked ? Theme.colors.tabPanelBackground :
            _control.hovered ? Theme.colors.tabButtonHover : Theme.colors.tabButtonInactive
    }

    property color textColor: {
        _control.checked ? Theme.colors.label :
            _control.hovered ? Theme.colors.label : Theme.colors.labelAlt
    }

    contentItem: Text {
        text: _control.text
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        color: _control.textColor
        font.pixelSize: 12
    }

    // Main background rectangle with rounded corners
    background: Rectangle {
        color: _control.backgroundColor
        radius: Theme.buttonCornerRadius
        border.width: _control.borderThickness
        border.color: _control.borderColor

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
                width: _control.borderThickness
                height: parent.height
                anchors.left: parent.left
                color: _control.borderColor
            }
            // The right border for bottom half
            Rectangle {
                width: _control.borderThickness
                height: parent.height
                anchors.right: parent.right
                color: _control.borderColor
            }
        }

        // The bottom border
        Rectangle {
            width: _control.checked ? parent.width - 4 : parent.width
            height: _control.borderThickness
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter

            color: _control.bottomBorder
            //Behavior on color {ColorAnimation{duration: 150}}
        }
    }
}