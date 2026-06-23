import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow
{
    id: window

    width: 400
    height: 250

    flags: Qt.Window | Qt.FramelessWindowHint
    visible: false
    color: "transparent"

    property int windowCornerRadius: 10 // Window corner radius

    background: WindowBackground { cornerRadius: window.windowCornerRadius }

    header: WindowTitleBar {
        windowTitleText: "Install mod"
        cornerRadius: window.windowCornerRadius
    }

}