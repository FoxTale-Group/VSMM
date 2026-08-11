import QtQuick
import QtQuick.Effects

Item {
    id: root
    property bool isRevealed: false
    property int animationDuration: 350

    default property alias content: contentContainer.children

    // What gets revealed. Rendered to a layer so MultiEffect can mask it.
    Item {
        id: contentContainer
        anchors.fill: parent
        visible: false
        layer.enabled: true
    }

    // The mask. Stays put at the content's size while the circle inside it grows.
    Item {
        id: maskContainer
        anchors.fill: parent
        visible: false       // keeps the oversized circle off screen
        layer.enabled: true  // keeps its texture live so it can serve as the mask

        Rectangle {
            id: expandingCircle
            anchors.centerIn: parent

            // Animate physical size rather than scale, which breaks at 0x0. The 2.5
            // factor guarantees the circle covers the far corners.
            width: root.isRevealed ? root.width * 2.5 : 0
            height: width
            radius: width / 2

            color: "black" // only the shape is used, never the colour

            Behavior on width {
                NumberAnimation {
                    duration: root.animationDuration
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    MultiEffect {
        source: contentContainer
        anchors.fill: contentContainer
        maskEnabled: true
        maskSource: maskContainer // the static container, not the moving circle
    }
}