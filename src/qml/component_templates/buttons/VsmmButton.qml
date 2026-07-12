import QtQml
import QtQuick
import QtQuick.Controls
import vsmm

Button {
    id: rootButton

    property color defaultColor: Theme.colors.buttonDefault
    property color hoverColor: Theme.colors.buttonHover
    property color pressColor: Theme.colors.buttonPress
    property int radius: Theme.buttonCornerRadius
    property string tooltipText: ""

    property string iconColor: Theme.colors.buttonIcon

    font.bold: true
    palette.buttonText: Theme.colors.buttonText
    icon.width: 30
    icon.height: 30
    icon.color: iconColor

    ToolTip.text: tooltipText
    ToolTip.visible: hovered && tooltipText !== ""
    ToolTip.delay: 500

    scale: rootButton.down ? 0.95 : 1.0

    Behavior on scale {NumberAnimation {duration: 100; easing.type: Easing.OutQuad}}

    background: Rectangle {
        radius: rootButton.radius

        color: {
            if (rootButton.down) {
                return rootButton.pressColor
            } else if (rootButton.hovered) {
                return rootButton.hoverColor
            } else {
                return rootButton.defaultColor
            }
        }

        Behavior on color {ColorAnimation {duration: 150; easing.type: Easing.OutCubic}}
    }

    HoverHandler {cursorShape: Qt.PointingHandCursor}
}