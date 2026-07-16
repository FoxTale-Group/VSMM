import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Window

Item {
    id: _TitleBar
    property string windowIconSource: ""
    property string windowTitleText: ""
    property int cornerRadius: 0

    property bool showMaximize: true
    property bool showMinimize: true

    property bool canMove: true

    height: 30

    // Title bar background rectangle with rounded corners
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: {
            if (Window.window.visibility === Window.Maximized) {
                return 0
            } else {
                return parent.cornerRadius
            }
        }
    }

    MouseArea {
        enabled: _TitleBar.canMove
        anchors.fill: parent
        onPressed: Window.window.startSystemMove()
        // Double-click to maximize/restore
        onDoubleClicked: {
            if (Window.window.visibility === Window.Maximized) {
                Window.window.showNormal()
            } else {
                Window.window.showMaximized()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.bottomMargin: 0
        anchors.topMargin: 3
        anchors.rightMargin: 3

        IconImage {
            source: _TitleBar.windowIconSource
            visible: !!_TitleBar.windowIconSource
            sourceSize: Qt.size(20, 20)
        }

        Text {
            text: _TitleBar.windowTitleText
            font.pixelSize: Theme.fonts.body
            color: Theme.colors.windowTitle
            Layout.fillWidth: true
        }

        // Minimize Button
        Button {
            icon.source: Theme.icons.minimizeIcon
            icon.color: Theme.colors.icon
            display: AbstractButton.IconOnly
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30

            visible: _TitleBar.showMinimize

            background: Rectangle {
                color: parent.hovered ? Theme.colors.modEntryButtonHover : "transparent"
                radius: 8
            }

            onClicked: Window.window.showMinimized()
        }

        // Maximize / Restore Button
        Button {
            // Change icon based on current state
            icon.source: Window.window.visibility === Window.Maximized ? Theme.icons.exitFullscreenIcon : Theme.icons.goFullscreenIcon
            icon.color: Theme.colors.icon
            display: AbstractButton.IconOnly
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30

            visible: _TitleBar.showMaximize

            background: Rectangle {
                color: parent.hovered ? Theme.colors.modEntryButtonHover : "transparent"
                radius: 8
            }

            onClicked: {
                if (Window.window.visibility === Window.Maximized) {
                    Window.window.showNormal()
                } else {
                    Window.window.showMaximized()
                }
            }
        }

        // Close Button
        Button {
            icon.source: Theme.icons.closeIcon
            icon.color: Theme.colors.icon
            display: AbstractButton.IconOnly
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30

            // Standard convention: Close buttons turn red on hover
            background: Rectangle {
                color: parent.hovered ? Theme.colors.winButtonCloseHover : "transparent"
                radius: 8
            }
            palette.buttonText: parent.hovered ? Theme.colors.winButtonCloseTextHover : Theme.colors.winButtonCloseText

            onClicked: Window.window.close()
        }
    }
}