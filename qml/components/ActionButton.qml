import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button
{
    id: rootButton

    property color defaultColor: "#595959"
    property color hoverColor: "#3e3e3e"
    property color pressColor: "#2a2a2a"
    property int radius: 6
    property string tooltipText: ""

    font.bold: true
    palette.buttonText: "white"
    icon.color: "white"
    icon.width: 30
    icon.height: 30

    ToolTip.text: tooltipText
    ToolTip.visible: hovered && tooltipText !== ""
    ToolTip.delay: 500

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
    }
}