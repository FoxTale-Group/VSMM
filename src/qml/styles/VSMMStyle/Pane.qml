import QtQuick
import QtQuick.Templates as T
import vsmm

T.Pane {
    id: control

    property int radius: Theme.radius.panel

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    padding: 12

    background: Rectangle {
        radius: control.radius
        color: Theme.colors.panelBg
        border.color: Theme.colors.panelBorder
        border.width: 1
    }
}
