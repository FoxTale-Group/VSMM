import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl // IconImage
import vsmm

T.CheckBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: 6
    spacing: 6

    font.pixelSize: Theme.fonts.body

    // SVG indicator — padding removed from select_check_box.svg (viewBox now
    // "120 -840 765 720", was "0 -960 960 960"). The checked glyph renders
    // edge-to-edge instead of inset, so it no longer looks smaller than the
    // unchecked outline. Requires import QtQuick.Controls.impl (IconImage).
    /*indicator: Item {
        implicitWidth: 20
        implicitHeight: 20
        // Center the box when there's no text, otherwise sit at the leading edge.
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
                        : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        // Unchecked: empty rounded box, brighter border on hover.
        Rectangle {
            anchors.fill: parent
            radius: Theme.radius.indicator
            color: "transparent"
            border.width: 2
            border.color: control.hovered ? Theme.colors.label : Theme.colors.buttonDefault
            visible: !control.checked
        }

        // Checked: the now padding-free glyph, tinted with the accent.
        IconImage {
            anchors.fill: parent
            source: Theme.icons.selectCheckBoxIcon
            sourceSize.width: 20
            sourceSize.height: 20
            color: Theme.colors.highlightButtonDefault
            visible: control.checked
        }
    }*/

    // ---------------------------------------------------------------------------
    // ALT (kept for comparison) — drawn persistent box + Canvas checkmark. No SVG,
    // no shrink at all (single Rectangle, only colours change between states).
    //
     indicator: Rectangle {
         id: box
         implicitWidth: 20
         implicitHeight: 20
         x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
                         : control.leftPadding + (control.availableWidth - width) / 2
         y: control.topPadding + (control.availableHeight - height) / 2
         radius: Theme.radius.indicator
         color: control.checked ? Theme.colors.highlightButtonDefault : "transparent"
         border.width: 2
         border.color: control.checked ? Theme.colors.highlightButtonDefault
                     : control.hovered ? Theme.colors.label
                     : Theme.colors.buttonDefault
         Canvas {
             anchors.fill: parent
             visible: control.checked
             onPaint: {
                 const ctx = getContext("2d");
                 ctx.reset();
                 ctx.strokeStyle = Theme.colors.icon;
                 ctx.lineWidth = 2; ctx.lineCap = "round"; ctx.lineJoin = "round";
                 ctx.beginPath();
                 ctx.moveTo(width * 0.24, height * 0.52);
                 ctx.lineTo(width * 0.42, height * 0.70);
                 ctx.lineTo(width * 0.76, height * 0.30);
                 ctx.stroke();
             }
             onVisibleChanged: if (visible) requestPaint()
         }
     }
    // ---------------------------------------------------------------------------

    contentItem: Text {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0
        text: control.text
        font: control.font
        color: Theme.colors.label
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
