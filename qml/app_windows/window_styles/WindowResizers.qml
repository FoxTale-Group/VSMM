import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Window

Item {
    parent: Overlay.overlay
    anchors.fill: parent

    // Top Edge
    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 5
        cursorShape: Qt.SizeVerCursor
        onPressed: Window.window.startSystemResize(Qt.TopEdge)
    }
    // Bottom Edge
    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 5
        cursorShape: Qt.SizeVerCursor
        onPressed: Window.window.startSystemResize(Qt.BottomEdge)
    }
    // Left Edge
    MouseArea {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 5
        cursorShape: Qt.SizeHorCursor
        onPressed: Window.window.startSystemResize(Qt.LeftEdge)
    }
    // Right Edge
    MouseArea {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 5
        cursorShape: Qt.SizeHorCursor
        onPressed: Window.window.startSystemResize(Qt.RightEdge)
    }
    // Top-Left Corner
    MouseArea {
        anchors.top: parent.top
        anchors.left: parent.left
        width: 8
        height: 8
        cursorShape: Qt.SizeFDiagCursor
        onPressed: Window.window.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
    }

    // Top-Right Corner
    MouseArea {
        anchors.top: parent.top
        anchors.right: parent.right
        width: 8
        height: 8
        cursorShape: Qt.SizeBDiagCursor
        onPressed: Window.window.startSystemResize(Qt.TopEdge | Qt.RightEdge)
    }

    // Bottom-Left Corner
    MouseArea {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 8
        height: 8
        cursorShape: Qt.SizeBDiagCursor
        onPressed: Window.window.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
    }

    // Bottom-Right Corner
    MouseArea {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 8
        height: 8
        cursorShape: Qt.SizeFDiagCursor
        onPressed: Window.window.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
    }
}