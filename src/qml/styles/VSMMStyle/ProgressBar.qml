import QtQuick
import QtQuick.Templates as T
import vsmm

T.ProgressBar {
    id: control

    property int thickness: 12
    property int borderWidth: 1
    property int fillGap: 2 // Space between the inside of the border and the fill.
    property int fillDuration: 120

    // Border + gap, so the fill 'floats' inside the border.
    padding: borderWidth + fillGap

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: control.thickness
        radius: height / 2
        color: Theme.colors.progressBarTrack
        border.color: Theme.colors.progressBarBorder
        border.width: control.borderWidth
    }

    contentItem: Item {
        implicitHeight: control.thickness - control.topPadding - control.bottomPadding

        // Fill follows window resizes
        property real shownPosition: control.visualPosition
        Behavior on shownPosition {
            NumberAnimation { duration: control.fillDuration; easing.type: Easing.OutCubic }
        }

        Rectangle {
            visible: !control.indeterminate && width > 0
            x: control.mirrored ? parent.width - width : 0
            width: parent.shownPosition * parent.width
            height: parent.height
            radius: height / 2
            color: Theme.colors.progressBarFill
        }

        // Indeterminate: a short pill sweeping end to end.
        Rectangle {
            id: sweep

            visible: control.indeterminate
            width: parent.width * 0.3
            height: parent.height
            radius: height / 2
            color: Theme.colors.progressBarFill

            SequentialAnimation on x {
                running: control.indeterminate && control.visible
                loops: Animation.Infinite
                NumberAnimation {
                    from: 0; to: sweep.parent.width - sweep.width
                    duration: 900; easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    from: sweep.parent.width - sweep.width; to: 0
                    duration: 900; easing.type: Easing.InOutQuad
                }
            }
        }
    }
}
