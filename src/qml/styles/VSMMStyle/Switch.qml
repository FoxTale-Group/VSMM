pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import vsmm

T.Switch {
    id: control

    // The inset is half the difference between track height and thumb, so the thumb
    // travels exactly its own width and the end margins stay even.
    property int trackWidth:  50
    property int trackHeight: 30
    property int thumbSize:   20
    property int thumbInset:   5

    // Stroke of the unchecked thumb's ring, and so the radius of the hole the track
    // shows through it.
    property int ringWidth: 5

    // CSS `ease`, cubic-bezier(0.25, 0.1, 0.25, 1.0).
    readonly property var easeCurve: [0.25, 0.1, 0.25, 1.0, 1.0, 1.0]
    property int animationDuration: 200

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding
                            + implicitIndicatorWidth + spacing)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             Math.max(implicitContentHeight, implicitIndicatorHeight)
                             + topPadding + bottomPadding)

    padding: 6
    spacing: 8
    font.pixelSize: Theme.fonts.body

    indicator: Rectangle {
        id: track

        implicitWidth: control.trackWidth
        implicitHeight: control.trackHeight

        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding
                                            : control.leftPadding)
                        : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        radius: height / 2
        color: control.checked ? Theme.colors.switchTrackOn : Theme.colors.switchTrackOff
        opacity: control.enabled ? 1.0 : 0.5

        Behavior on color {
            ColorAnimation {
                duration: control.animationDuration
                easing.type: Easing.Bezier
                easing.bezierCurve: control.easeCurve
            }
        }

        Rectangle {
            id: thumb

            width: control.thumbSize
            height: control.thumbSize
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter

            x: control.checked ? track.width - width - control.thumbInset
                               : control.thumbInset

            // Off: transparent centre inside a solid ring, so the track reads through.
            // On: the centre fills and the ring becomes a dot.
            color: control.checked ? Theme.colors.switchThumb : "transparent"
            border.width: control.ringWidth
            border.color: Theme.colors.switchThumb

            Behavior on x {
                NumberAnimation {
                    duration: control.animationDuration
                    easing.type: Easing.Bezier
                    easing.bezierCurve: control.easeCurve
                }
            }
            Behavior on color {
                ColorAnimation {
                    duration: control.animationDuration
                    easing.type: Easing.Bezier
                    easing.bezierCurve: control.easeCurve
                }
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
