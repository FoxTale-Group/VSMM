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

    property string buttonIcon: ""
    property string iconColor: "#ffffff"

    icon.source: buttonIcon ? "qrc:/qt/qml/vsmm/assets/icons/" + buttonIcon : ""

    font.bold: true
    icon.width: 20
    icon.height: 20

    ToolTip.text: tooltipText
    ToolTip.visible: hovered && tooltipText !== ""
    ToolTip.delay: 500

    display: AbstractButton.IconOnly

    scale: rootButton.down ? 0.95 : 1.0

    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }

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

        Behavior on color {
            ColorAnimation {
                duration: 150
                easing.type: Easing.OutCubic
            }
        }
    }

    contentItem: Item {
        implicitWidth: rootButton.icon.width
        implicitHeight: rootButton.icon.height

        IconImage {
            anchors.centerIn: parent

            source: rootButton.icon.source
            color: rootButton.iconColor
            
            sourceSize: Qt.size(rootButton.icon.width, rootButton.icon.height)
            fillMode: Image.PreserveAspectFit
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
}