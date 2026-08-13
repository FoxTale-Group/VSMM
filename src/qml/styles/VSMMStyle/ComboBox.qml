pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import vsmm

T.ComboBox {
    id: control

    property int radius: Theme.radius.field

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 6
    leftPadding: padding + 6
    rightPadding: padding + 6
    spacing: 8

    font.pixelSize: Theme.fonts.label

    // Groove matches TextField: same fill, same focus border, so the two input
    // controls read as one family.
    background: Rectangle {
        implicitHeight: 34
        radius: control.radius
        color: control.enabled ? Theme.colors.searchBarBg : Theme.colors.buttonInactive
        border.width: control.activeFocus || control.popup.visible ? 1 : 0
        border.color: Theme.colors.searchBarIcon

        Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
    }

    contentItem: Text {
        leftPadding: 0
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        font: control.font
        color: control.enabled ? Theme.colors.text : Theme.colors.labelVersion
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
        elide: Text.ElideRight
    }

    // No chevron exists in the icon set, so it is drawn. Rotates 180° while the
    // popup is open.
    indicator: Item {
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 12
        height: 12

        rotation: control.popup.visible ? 180 : 0
        Behavior on rotation { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

        Canvas {
            id: chevron
            anchors.fill: parent

            readonly property color stroke: control.enabled ? Theme.colors.searchBarDefault
                                                            : Theme.colors.buttonInactive
            onStrokeChanged: requestPaint()
            onVisibleChanged: if (visible) requestPaint()

            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                ctx.strokeStyle = chevron.stroke;
                ctx.lineWidth = 2;
                ctx.lineCap = "round";
                ctx.lineJoin = "round";
                ctx.beginPath();
                ctx.moveTo(width * 0.2, height * 0.38);
                ctx.lineTo(width * 0.5, height * 0.68);
                ctx.lineTo(width * 0.8, height * 0.38);
                ctx.stroke();
            }
        }
    }

    delegate: T.ItemDelegate {
        id: option

        required property var model
        required property int index

        width: ListView.view.width
        height: 32
        padding: 6
        leftPadding: padding + 6
        rightPadding: padding + 6

        text: option.model[control.textRole] ?? option.model.modelData ?? ""
        font: control.font
        highlighted: control.highlightedIndex === option.index
        hoverEnabled: control.hoverEnabled

        contentItem: Text {
            text: option.text
            font: option.font
            // The selected entry keeps the accent so it stays findable while
            // the hover highlight moves around it.
            color: control.currentIndex === option.index ? Theme.colors.highlightButton
                                                         : Theme.colors.text
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: Theme.radius.badge
            // Lightens the popup's own surface, so the row rises toward the cursor rather
            // than darkening against the panel behind it.
            color: option.highlighted || option.hovered
                   ? Theme.shade(Theme.colors.tabPanelBackground, -Theme.hoverShade)
                   : "transparent"

            Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
        }
    }

    popup: T.Popup {
        y: control.height + 4
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, 240)
        padding: 4

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {}
        }

        background: Rectangle {
            radius: Theme.radius.card
            color: Theme.colors.tabPanelBackground
            border.width: 1
            border.color: Theme.colors.tabPanelBorder
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 150; easing.type: Easing.OutQuad }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 120; easing.type: Easing.InQuad }
        }
    }
}
