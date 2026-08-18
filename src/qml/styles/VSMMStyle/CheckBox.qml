pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import VSMM.Theme

T.CheckBox {
    id: control

    property int boxSize: 22

    // CSS `ease`, cubic-bezier(0.25, 0.1, 0.25, 1.0).
    readonly property var easeCurve: [0.25, 0.1, 0.25, 1.0, 1.0, 1.0]

    // Geometry morphs slowly while the strokes swap over faster, so the outline has
    // given way to the tick well before the shape settles.
    property int morphDuration: 250
    property int strokeDuration: 100

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding
                            + implicitIndicatorWidth + spacing)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             Math.max(implicitContentHeight, implicitIndicatorHeight)
                             + topPadding + bottomPadding)

    padding: 6
    spacing: 6
    font.pixelSize: Theme.fonts.body

    indicator: Rectangle {
        id: box

        implicitWidth: control.boxSize
        implicitHeight: control.boxSize

        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding
                                            : control.leftPadding)
                        : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        radius: Theme.radius.indicator
        color: control.checked ? Theme.colors.checkBoxChecked : "transparent"
        opacity: control.enabled ? 1.0 : 0.5

        Behavior on color {
            ColorAnimation {
                duration: control.morphDuration
                easing.type: Easing.Bezier
                easing.bezierCurve: control.easeCurve
            }
        }

        // Both states are the same element. Unchecked it is a rounded-square outline
        // filling the box; checked it is a narrow bar rotated 45° showing only its right
        // and bottom edges, which reads as a tick. Every dimension is a fraction of
        // `boxSize` so the shape scales with the control.
        Item {
            id: morph

            readonly property real legWidth: control.boxSize * 0.115

            x:      control.checked ? control.boxSize * 0.346 : 0
            y:      control.checked ? control.boxSize * 0.192 : 0
            width:  control.checked ? control.boxSize * 0.308 : control.boxSize * 0.962
            height: control.checked ? control.boxSize * 0.500 : control.boxSize * 0.962
            rotation: control.checked ? 45 : 0

            Behavior on x        { NumberAnimation { duration: control.morphDuration; easing.type: Easing.Bezier; easing.bezierCurve: control.easeCurve } }
            Behavior on y        { NumberAnimation { duration: control.morphDuration; easing.type: Easing.Bezier; easing.bezierCurve: control.easeCurve } }
            Behavior on width    { NumberAnimation { duration: control.morphDuration; easing.type: Easing.Bezier; easing.bezierCurve: control.easeCurve } }
            Behavior on height   { NumberAnimation { duration: control.morphDuration; easing.type: Easing.Bezier; easing.bezierCurve: control.easeCurve } }
            Behavior on rotation { NumberAnimation { duration: control.morphDuration; easing.type: Easing.Bezier; easing.bezierCurve: control.easeCurve } }

            // The unchecked outline. Rectangle has no per-side borders, so it fades out
            // whole on the stroke timing rather than thinning two of its edges away.
            Rectangle {
                anchors.fill: parent
                radius: control.checked ? 0 : Theme.radius.indicator
                color: "transparent"
                border.width: control.boxSize * 0.077
                border.color: control.hovered ? Theme.colors.checkBoxBorderHover
                                              : Theme.colors.checkBoxBorder
                opacity: control.checked ? 0.0 : 1.0

                Behavior on opacity { NumberAnimation { duration: control.strokeDuration } }
                Behavior on radius  { NumberAnimation { duration: control.morphDuration; easing.type: Easing.Bezier; easing.bezierCurve: control.easeCurve } }
                Behavior on border.color { ColorAnimation { duration: control.strokeDuration } }
            }

            // The two edges that survive into the checked state.
            Rectangle {
                anchors.right: parent.right
                width: morph.legWidth
                height: parent.height
                color: Theme.colors.checkBoxMark
                opacity: control.checked ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: control.strokeDuration } }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: morph.legWidth
                color: Theme.colors.checkBoxMark
                opacity: control.checked ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: control.strokeDuration } }
            }
        }
    }

    contentItem: Text {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: Theme.colors.label
        opacity: control.enabled ? 1.0 : 0.5
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
