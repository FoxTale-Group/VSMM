import QtQml
import QtQuick
import QtQuick.Controls

import VSMM.Theme
import VSMM.WindowComponents

ApplicationWindow
{
    id: vsmmWindow

    // Overwritable window parameters
    width: 400
    height: 250
    minimumWidth: 400
    minimumHeight: 250

    // Custom properties
    property string windowTitle: ""
    property string windowIcon: ""

    // Window type properties
    property bool dialog: false
    property bool movable: true
    property bool resizable: true
    property bool minimizable: !dialog

    visible: !dialog

    flags: dialog ? (Qt.Dialog | Qt.FramelessWindowHint) : (Qt.Window | Qt.FramelessWindowHint)
    modality: Qt.NonModal // Qt.NonModal, Qt.WindowModal, Qt.ApplicationModal

    color: "transparent"

    header: WindowTitleBar {
        windowTitleText: vsmmWindow.windowTitle
        windowIconSource: vsmmWindow.windowIcon
        cornerRadius: Theme.radius.window

        showMaximize: vsmmWindow.resizable
        showMinimize: vsmmWindow.minimizable

        canMove: vsmmWindow.movable
    }

    // Window drag area
    MouseArea {
        enabled: vsmmWindow.movable
        anchors.fill: parent
        onPressed: (mouse) => {
            vsmmWindow.startSystemMove()
            mouse.accepted = true
        }
    }

    WindowResizers {
        enabled: vsmmWindow.resizable
        visible: vsmmWindow.resizable
    }

    background: Rectangle {
        anchors.fill: parent
        color: Theme.colors.windowBackground
        radius: Window.window.visibility === Window.Maximized ? 0 : Theme.radius.window

        border.color: Theme.colors.windowBorder
        border.width: Window.window.visibility === Window.Maximized ? 0 : 1
    }
}