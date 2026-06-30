import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

VsmmWindow {
    id: _settingsWindow

    width: 600
    height: 400
    minimumWidth: 600
    minimumHeight: 400

    windowTitle: "Settings"
    windowIcon: "icons/settings.svg"

    dialog: true
    movable: false
    modality: Qt.ApplicationModal
}