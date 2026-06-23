import QtQuick
import QtQuick.Window

Rectangle {
    property int cornerRadius: 0

    anchors.fill: parent
    color: "#1e1e1e" // App background color
    radius: {
        if (Window.window.visibility === Window.Maximized) {
            return 0
        } else {
            return cornerRadius
        }
    }

    border.color: "#555555"
    border.width: {
        if (Window.window.visibility === Window.Maximized) {
            return 0
        } else {
            return 1 // If window_elements is not maximized, display border
        }
    }
}