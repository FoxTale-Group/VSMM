import QtQml
import QtQuick
import QtQuick.Controls

ApplicationWindow
{
    id: addModWindow

    width: 400
    height: 250

    flags: Qt.Dialog | Qt.FramelessWindowHint
    modality: Qt.ApplicationModal

    visible: false
    color: "transparent"

    property int windowCornerRadius: 10 // Window corner radius

    background: WindowBackground { cornerRadius: addModWindow.windowCornerRadius }

    header: WindowTitleBar {
        windowTitleText: "Install mod"
        cornerRadius: addModWindow.windowCornerRadius
        showMinimize: false
        showMaximize: false
    }

    AddModWindowContent{}
}