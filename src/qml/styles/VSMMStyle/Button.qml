import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl // IconLabel
import vsmm

T.Button {
    id: rootButton

    // Set `modEntry: true` for the mod-row icon button variant.
    property bool modEntry: false

    // Set `defaultColor` alone: hover and press are derived from it, so a call site needs
    // one colour per button. Both remain assignable for roles that are not a lightness
    // ramp -- the modEntry variant among them, whose transparent base has nothing to shade.
    property color defaultColor: modEntry ? Theme.colors.modEntryButtonDefault : Theme.colors.buttonDefault
    property color hoverColor:   modEntry ? Theme.colors.modEntryButtonHover
                                         : Theme.shade(rootButton.defaultColor, Theme.hoverShade)
    property color pressColor:   modEntry ? Theme.colors.modEntryButtonPress
                                         : Theme.shade(rootButton.defaultColor, Theme.pressShade)
    property color inactiveColor: modEntry ? Theme.colors.modEntryButtonDefault : Theme.colors.buttonInactive
    property color iconColor:    modEntry ? Theme.colors.icon                  : Theme.colors.buttonIcon
    property int   radius:       Theme.radius.button
    property string tooltipText: ""

    // Sizing calculations
    implicitWidth:  Math.max(implicitBackgroundWidth + leftInset + rightInset,
                             implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 8
    spacing: 6

    font.pixelSize: Theme.fonts.label
    font.bold: true
    icon.width:  modEntry ? 20 : 30
    icon.height: modEntry ? 20 : 30
    icon.color:  iconColor
    display:     modEntry ? T.AbstractButton.IconOnly : T.AbstractButton.TextBesideIcon

    scale: rootButton.down ? 0.95 : 1.0
    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

    // ToolTip's attached type lives in QtQuick.Templates here (a style must not import
    // QtQuick.Controls), so it must be qualified as T.ToolTip, not the bare name.
    T.ToolTip.text: tooltipText
    T.ToolTip.visible: hovered && tooltipText !== ""
    T.ToolTip.delay: 500

    contentItem: IconLabel {
        spacing: rootButton.spacing
        mirrored: rootButton.mirrored
        display: rootButton.display
        icon: rootButton.icon
        text: rootButton.text
        font: rootButton.font
        color: Theme.colors.buttonText
    }

    background: Rectangle {
        radius: rootButton.radius
        color: !rootButton.enabled ? rootButton.inactiveColor
             : rootButton.down    ? rootButton.pressColor
             : rootButton.hovered ? rootButton.hoverColor
             : rootButton.defaultColor
        Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
