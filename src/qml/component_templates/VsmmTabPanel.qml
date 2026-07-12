import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import vsmm

Rectangle {
    color: "transparent"

    property alias content: innerContent.data

    // Background
    Rectangle {
        anchors.fill: parent

        color: Theme.colors.tabPanelBackground
        radius: Theme.windowRadius
        border.width: 2
        border.color: Theme.colors.tabPanelBorder

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            width: parent.width
            height: parent.height - parent.radius

            color: parent.color

            // The left border for bottom half
            Rectangle {
                width: 2
                height: parent.height
                anchors.left: parent.left
                color: Theme.colors.tabPanelBorder
            }
            // The right border for bottom half
            Rectangle {
                width: 2
                height: parent.height
                anchors.right: parent.right
                color: Theme.colors.tabPanelBorder
            }
        }
    }

    Item {
        id: innerContent
        anchors.fill: parent
        anchors.margins: 10
    }
}