pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import vsmm

T.BusyIndicator {
    id: control

    property int segments: 12
    property int cycleDuration: 1000
    property int tailLength: 4
    property real restOpacity: 0.3

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding

    padding: 0

    contentItem: Item {
        id: spinner

        implicitWidth: 24
        implicitHeight: 24

        readonly property real size: Math.min(width, height)
        // Index of the leading segment, advanced continuously and floored per segment.
        property real phase: 0

        opacity: control.running ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { OpacityAnimator { duration: 250 } }

        NumberAnimation on phase {
            from: 0; to: control.segments
            duration: control.cycleDuration
            loops: Animation.Infinite
            running: spinner.visible
        }

        Repeater {
            model: control.segments

            Rectangle {
                id: segment

                required property int index
                // How many steps this segment trails the leading one, 0 for the leader.
                readonly property int lag: (Math.floor(spinner.phase) - index + control.segments) % control.segments

                x: (spinner.width - width) / 2
                y: (spinner.height - spinner.size) / 2
                width: Math.max(1.5, spinner.size * 0.075)
                height: spinner.size * 0.2
                radius: width / 2

                color: Theme.colors.busyIndicator
                opacity: lag >= control.tailLength
                         ? control.restOpacity
                         : 1 - (1 - control.restOpacity) * lag / control.tailLength

                transform: Rotation {
                    origin.x: segment.width / 2
                    origin.y: spinner.size / 2
                    angle: segment.index * 360 / control.segments
                }
            }
        }
    }
}
