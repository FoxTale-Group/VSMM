import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

Button
{
    id: rootButton

    property color defaultColor: "#595959"
    property color hoverColor: "#3e3e3e"
    property color pressColor: "#2a2a2a"
    property int radius: 6
    property string tooltipText: ""

    property string iconColor: "#ffffff"

    font.bold: true
    icon.width: 30
    icon.height: 30

    ToolTip.text: tooltipText
    ToolTip.visible: hovered && tooltipText !== ""
    ToolTip.delay: 500

    display: AbstractButton.IconOnly

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

    contentItem: IconImage {
        source: rootButton.icon.source
        color: rootButton.iconColor

        width: rootButton.icon.width
        height: rootButton.icon.height

        sourceSize.width: rootButton.icon.width
        sourceSize.height: rootButton.icon.height

        anchors.centerIn: parent
    }
}