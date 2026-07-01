import QtQml
import QtQuick
import QtQuick.Controls

ApplicationWindow
{
    id: _VsmmWindow

    // Overwritable window parameters
    width: 400
    height: 250
    minimumWidth: 400
    minimumHeight: 250

    // Custom properties
    property string windowTitle: "Window Title"
    property string windowIcon: ""
    property int windowCornerRadius: 10

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
        windowTitleText: _VsmmWindow.windowTitle
        windowIconSource: _VsmmWindow.windowIcon ? "qrc:/qt/qml/vsmm/assets/" + _VsmmWindow.windowIcon : ""
        cornerRadius: _VsmmWindow.windowCornerRadius

        showMaximize: _VsmmWindow.resizable
        showMinimize: _VsmmWindow.minimizable

        canMove: _VsmmWindow.movable
    }

    // Window drag area
    MouseArea {
        enabled: _VsmmWindow.movable
        anchors.fill: parent
        onPressed: (mouse) => {
            _VsmmWindow.startSystemMove()
            mouse.accepted = true
        }
    }

    WindowResizers {
        enabled: _VsmmWindow.resizable
        visible: _VsmmWindow.resizable
    }

    background: WindowBackground { cornerRadius: _VsmmWindow.windowCornerRadius }
}