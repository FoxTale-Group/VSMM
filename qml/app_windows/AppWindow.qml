import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmodchecker

ApplicationWindow
{
    id: windowMain

    width: 800
    height: 640
    minimumWidth: 750
    minimumHeight: 450

    flags: Qt.Window | Qt.FramelessWindowHint
    visible: true
    color: "transparent"

    property int windowCornerRadius: 10 // Window corner radius

    background: WindowBackground { cornerRadius: windowMain.windowCornerRadius }

    header: WindowTitleBar {
        windowTitleText: Qt.application.name + "  v" + Qt.application.version
        cornerRadius: windowMain.windowCornerRadius
    }

    WindowResizers{}

    AppWindowContent{}
}