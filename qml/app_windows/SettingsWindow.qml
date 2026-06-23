import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow
{
    id: window

    width: 600
    height: 400
    minimumWidth: 600
    minimumHeight: 400

    flags: Qt.Window | Qt.FramelessWindowHint
    visible: false
    color: "transparent"

    property int windowCornerRadius: 10 // Window corner radius

    background: WindowBackground { cornerRadius: window.windowCornerRadius }

    header: WindowTitleBar {
        windowTitleText: "Settings"
        cornerRadius: window.windowCornerRadius
    }

    WindowResizers{}
}