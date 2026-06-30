import QtQuick
import QtQuick.Effects

Item {
    id: root
    property bool isRevealed: false
    property int animationDuration: 350

    default property alias content: contentContainer.children

    // 1. THE CONTENT
    Item {
        id: contentContainer
        anchors.fill: parent
        visible: false
        layer.enabled: true
    }

    // 2. THE MASK CONTAINER (Static layer, hidden from UI)
    Item {
        id: maskContainer
        anchors.fill: parent
        visible: false       // CRITICAL: Hides the giant blob from the screen!
        layer.enabled: true  // CRITICAL: Keeps the texture active for the mask

        // 3. THE ANIMATED SHAPE
        Rectangle {
            id: expandingCircle
            anchors.centerIn: parent

            // Animate physical size instead of scale to prevent 0x0 GPU bugs!
            // Multiplied by 2.5 to guarantee it covers wide corners
            width: root.isRevealed ? root.width * 2.5 : 0
            height: width
            radius: width / 2

            color: "black" // Color doesn't matter, only the shape does

            Behavior on width {
                NumberAnimation {
                    duration: root.animationDuration
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    // 4. THE RENDERER
    MultiEffect {
        source: contentContainer
        anchors.fill: contentContainer
        maskEnabled: true
        maskSource: maskContainer // Target the static container, not the moving circle!
    }
}