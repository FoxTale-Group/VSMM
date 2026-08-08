import QtQuick
import QtQuick.Templates as T
import vsmm

T.ScrollBar {
    id: control

    property int thickness: 16

    // Inset at both ends of the track. ScrollBar sizes its contentItem inside
    // availableWidth/availableHeight, which subtract padding, so padding along the
    // scroll axis is what produces the gap.
    property int edgeMargin: 4

    topPadding:    control.horizontal ? 0 : control.edgeMargin
    bottomPadding: control.horizontal ? 0 : control.edgeMargin
    leftPadding:   control.horizontal ? control.edgeMargin : 0
    rightPadding:  control.horizontal ? control.edgeMargin : 0

    implicitWidth: Math.max(implicitContentWidth + leftPadding + rightPadding,
                            control.horizontal ? 0 : thickness)
    implicitHeight: Math.max(implicitContentHeight + topPadding + bottomPadding,
                             control.horizontal ? thickness : 0)

    // Honor the AlwaysOff policy; the design has no fade, so AsNeeded just shows the bar.
    visible: control.policy !== T.ScrollBar.AlwaysOff

    background: Item {}

    // The contentItem spans the full `thickness` so the whole track stays grabbable; the
    // visible bar inside it is half that and centred, so the handle floats in the middle.
    contentItem: Item {
        implicitWidth:  control.horizontal ? 0 : control.thickness
        implicitHeight: control.horizontal ? control.thickness : 0

        Rectangle {
            anchors.centerIn: parent

            width:  control.horizontal ? parent.width       : parent.width * 0.5
            height: control.horizontal ? parent.height * 0.5 : parent.height

            // Keyed off the drawn size, not `thickness`, so a short handle stays a pill
            // rather than over-rounding into a circle.
            radius: Math.min(width, height) / 2

            color: control.pressed ? Theme.colors.scrollBarHandlePress
                 : control.hovered ? Theme.colors.scrollBarHandleHover
                 : Theme.colors.scrollBarHandleDefault
        }
    }
}
