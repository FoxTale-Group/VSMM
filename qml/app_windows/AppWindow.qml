import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import main

ApplicationWindow
{
    id: windowMain

    //region WINDOW SIZE & STYLE
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
    //endregion


    property int updatesAvailable: {
        let c = 0;
        for (let i = 0; i < ModListModel.count; i++)
            if (ModListModel.get(i).updateVersion !== "latest") c++
        return c
    }

    AppWindowContent{}
}