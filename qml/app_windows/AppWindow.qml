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

    MouseArea {
        anchors.fill: parent
        onPressed: (mouse) => {
            appContent.forceActiveFocus()
            windowMain.startSystemMove()
            mouse.accepted = true
        }
    }

    WindowResizers{}

    SettingsWindow {
        id: settingsWindow
    }

    AddModWindow {
        id: addModWindow
    }

    AppWindowContent{
        id: appContent
        
        onOpenSettingsClicked: {
            settingsWindow.show()
            settingsWindow.raise()
            settingsWindow.requestActivate()
        }

        onRelayOpenAddModDialog: {
            addModWindow.show()
            addModWindow.raise()
            addModWindow.requestActivate()
        }
    }
}