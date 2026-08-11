pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl // IconImage
import vsmm

T.TextField {
    id: control

    // --- extension properties ---
    property url iconSource: ""
    property int radius: Theme.radius.field
    // Resting underline inset, so the line stops short of the rounded corners.
    property int underlineLeftMargin: 6
    property int underlineRightMargin: 16

    // Material's standard easing, cubic-bezier(0.4, 0, 0.2, 1).
    readonly property var easeCurve: [0.4, 0.0, 0.2, 1.0, 1.0, 1.0]
    property int revealDuration: 260
    property int labelDuration: 150

    // The label sits on the placeholder line when resting and floats into the upper
    // band of the field once there is focus or content.
    readonly property bool labelRaised: control.activeFocus || control.length > 0

    // Shared by the icon and the floating label so the two can never disagree. White
    // while focused, since the accent reads muddy against the filled panel; accent while
    // unfocused but populated, which is what marks a filled field at a glance.
    readonly property color labelColor: control.activeFocus ? Theme.colors.searchBarIcon
                                      : control.length > 0  ? Theme.colors.searchBarAccent
                                                            : Theme.colors.searchBarDefault

    // Top edge to the raised label, and the label down to the text.
    property int labelTopMargin: 2
    property int labelGap: 3

    // Real metrics for the raised label's height, so `topPadding` follows the meta token
    // and the font family. The sample string is fixed so the reserved band does not
    // jitter as the user types.
    TextMetrics {
        id: raisedMetrics
        font.family: control.font.family
        font.pixelSize: Theme.fonts.meta
        text: "Ag"
    }

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, 120) + leftPadding + rightPadding
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             40)

    // Reserve exactly the raised label's band plus a gap, so the two never overlap.
    topPadding: control.labelTopMargin + raisedMetrics.height + control.labelGap
    bottomPadding: 4
    leftPadding: (control.iconSource.toString() !== "" ? 34 : 12)
    rightPadding: 12

    font.pixelSize: Theme.fonts.label
    color: Theme.colors.text
    selectionColor: Theme.colors.textSelection
    selectedTextColor: Theme.colors.textSelected
    verticalAlignment: TextInput.AlignVCenter

    background: Item {
        implicitHeight: 40

        // Resting state: a hairline underline inset from the corners.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: control.underlineLeftMargin
            anchors.rightMargin: control.underlineRightMargin
            anchors.bottomMargin: 2
            height: 1
            color: Theme.colors.searchBarDefault
            opacity: control.activeFocus ? 0.0 : 1.0

            Behavior on opacity { NumberAnimation { duration: control.labelDuration } }
        }

        // Focus state: a bar at the bottom edge that grows upward to fill the field.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            height: control.activeFocus ? parent.height : 2
            radius: control.radius
            color: Theme.colors.searchBarBg
            opacity: control.activeFocus ? 1.0 : 0.0

            Behavior on height {
                NumberAnimation {
                    duration: control.revealDuration
                    easing.type: Easing.Bezier
                    easing.bezierCurve: control.easeCurve
                }
            }
            Behavior on opacity { NumberAnimation { duration: control.labelDuration } }
        }
    }

    // Leading icon, tinted with focus like the label.
    IconImage {
        id: leadingIcon

        visible: control.iconSource.toString() !== ""
        source: control.iconSource
        sourceSize: Qt.size(18, 18)
        width: 18
        height: 18

        x: 10
        anchors.verticalCenter: parent.verticalCenter

        color: control.labelColor
        Behavior on color { ColorAnimation { duration: control.labelDuration } }
    }

    // The placeholder doubles as the floating label: it shrinks and rises into the
    // upper band instead of disappearing on first keystroke.
    Text {
        id: floatingLabel

        text: control.placeholderText
        font.pixelSize: control.labelRaised ? Theme.fonts.meta : control.font.pixelSize
        color: control.labelColor

        x: control.leftPadding
        y: control.labelRaised
           ? control.labelTopMargin
           : (control.height - height) / 2
        width: control.width - control.leftPadding - control.rightPadding
        elide: Text.ElideRight

        Behavior on y {
            NumberAnimation {
                duration: control.labelDuration
                easing.type: Easing.Bezier
                easing.bezierCurve: control.easeCurve
            }
        }
        Behavior on font.pixelSize {
            NumberAnimation {
                duration: control.labelDuration
                easing.type: Easing.Bezier
                easing.bezierCurve: control.easeCurve
            }
        }
        Behavior on color { ColorAnimation { duration: control.labelDuration } }
    }

    // Escape gives focus back without the call site wiring it every time.
    Keys.onEscapePressed: (event) => {
        control.focus = false;
        event.accepted = true;
    }
}
