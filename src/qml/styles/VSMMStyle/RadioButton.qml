pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import VSMM.Theme

T.RadioButton {
    id: control

    // --- extension property ---
    // Set to paint the indicator as a flat colour swatch (the accent picker). Unset,
    // the indicator is a conventional dot in a ring.
    property color swatchColor: "transparent"
    readonly property bool isSwatch: control.swatchColor.a > 0

    property int swatchSize: 22
    property int ringGap: 3
    property int ringWidth: 2

    // Material's standard easing, matching the other VSMMStyle controls.
    readonly property var easeCurve: [0.4, 0.0, 0.2, 1.0, 1.0, 1.0]
    property int animationDuration: 150

    // The indicator is a separate term rather than being added to the content width:
    // contentItem already reserves the indicator in its own padding when there is text, so
    // adding both would count it twice. With no text the indicator term is what sizes the
    // control, which keeps a bare swatch down to its own footprint.
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitIndicatorWidth + leftPadding + rightPadding,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             Math.max(implicitContentHeight, implicitIndicatorHeight)
                             + topPadding + bottomPadding)

    padding: 4
    spacing: 6
    font.pixelSize: Theme.fonts.body

    indicator: Item {
        // The ring's footprint is always reserved and only its opacity and scale change,
        // so selecting a swatch does not nudge the row.
        implicitWidth: control.swatchSize + 2 * (control.ringGap + control.ringWidth)
        implicitHeight: implicitWidth

        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding
                                            : control.leftPadding)
                        : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        // Selection ring, outside the fill so it never obscures the colour being previewed.
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            border.width: control.ringWidth
            border.color: control.checked ? Theme.colors.radioRing : Theme.colors.radioRingHover

            opacity: control.checked ? 1.0 : (control.hovered ? 1.0 : 0.0)
            scale: control.checked ? 1.0 : 0.86

            Behavior on opacity { NumberAnimation { duration: control.animationDuration } }
            Behavior on border.color { ColorAnimation { duration: control.animationDuration } }
            Behavior on scale {
                NumberAnimation {
                    duration: control.animationDuration
                    easing.type: Easing.Bezier
                    easing.bezierCurve: control.easeCurve
                }
            }
        }

        // The swatch itself: a flat fill of the accent under preview.
        Rectangle {
            anchors.centerIn: parent
            width: control.swatchSize
            height: control.swatchSize
            radius: width / 2
            visible: control.isSwatch
            color: control.swatchColor
            opacity: control.enabled ? 1.0 : 0.5
        }

        // Fallback for a RadioButton used without a swatch colour.
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            visible: !control.isSwatch
            color: "transparent"
            border.width: 2
            border.color: control.hovered ? Theme.colors.checkBoxBorderHover
                                          : Theme.colors.checkBoxBorder

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.5
                height: width
                radius: width / 2
                color: Theme.colors.checkBoxChecked
                opacity: control.checked ? 1.0 : 0.0
                scale: control.checked ? 1.0 : 0.6

                Behavior on opacity { NumberAnimation { duration: control.animationDuration } }
                Behavior on scale {
                    NumberAnimation {
                        duration: control.animationDuration
                        easing.type: Easing.Bezier
                        easing.bezierCurve: control.easeCurve
                    }
                }
            }
        }
    }

    contentItem: Text {
        // Only reserved when there is a label to keep clear of the indicator; a swatch-only
        // button would otherwise carry the indicator's width as empty padding.
        leftPadding: control.text && control.indicator && !control.mirrored
                     ? control.indicator.width + control.spacing : 0
        rightPadding: control.text && control.indicator && control.mirrored
                      ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: Theme.colors.label
        opacity: control.enabled ? 1.0 : 0.5
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
