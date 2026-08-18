import QtQuick
import QtQuick.Templates as T
import VSMM.Theme

T.TabBar {
    id: control

    spacing: 5

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    contentItem: ListView {
        model: control.contentModel
        currentIndex: control.currentIndex
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.AutoFlickIfNeeded
        snapMode: ListView.SnapToItem
        highlightMoveDuration: 0
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: 40
        preferredHighlightEnd: width - 40
    }

    background: Rectangle {
        color: Theme.colors.windowBackground

        // A subtle separator line under the whole tab bar
        Rectangle {
            width: parent.width
            height: 2
            anchors.bottom: parent.bottom
            color: Theme.colors.tabPanelBorder
        }
    }
}
