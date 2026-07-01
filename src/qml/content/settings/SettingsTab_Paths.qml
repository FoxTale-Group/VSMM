import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import vsmm

Rectangle {
    id: _SettingsTab_Paths
    color: "transparent"

    ColumnLayout {
        anchors.fill: parent; spacing: 5;

        Label {text: "Game config folder"; font.pixelSize: 14; font.bold: true; color: "white"}

        RowLayout {
            Layout.fillWidth: true; spacing: 10

            Rectangle {
                id: modDirPath
                Layout.fillWidth: true; Layout.preferredHeight: 40; radius: 8

                property bool fieldFocused: modDirPath.activeFocus
                color: "transparent"

                Rectangle {anchors.fill: parent; radius: modDirPath.radius; color: "#3e3e3e"}

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 10; spacing: 5

                    IconImage {
                        source: "qrc:/qt/qml/vsmm/assets/icons/extension.svg"
                        color: modDirPath.fieldFocused ? "#ffffff" : "#888888"

                        Behavior on color {ColorAnimation {duration: 250}}
                    }

                    TextField {
                        id: modsDirInputField
                        Layout.fillWidth: true
                        placeholderText: "Provide path to Vintage Story config folder..."
                        color: "white"; font.pixelSize: 14

                        background: Rectangle {anchors.fill: parent; radius: modDirPath.radius; color: "#3e3e3e"}

                        Keys.onEscapePressed: (event) => {
                            focus = false
                            event.accepted = true
                        }
                    }
                }
            }

            ActionButton {
                buttonIcon: "folder.svg"
                display: AbstractButton.IconOnly
                Layout.preferredHeight: 40

                tooltipText: "Pick a folder"

                onClicked: {}
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "Current Path: "
                font.pixelSize: 12; font.bold: true; color: "green";
            }
            TextEdit {
                text: Config.modsDir

                readOnly: true; selectByMouse: true
                wrapMode: Text.WordWrap

                font.pixelSize: 12; color: "white"
                selectionColor: "lightblue"; selectedTextColor: "black"
            }
        }

        Label {text: "Game executable path"; font.pixelSize: 14; font.bold: true; color: "white"}

        RowLayout {
            Layout.fillWidth: true; spacing: 10

            Rectangle {
                id: gameExePath
                Layout.fillWidth: true; Layout.preferredHeight: 40; radius: 8

                property bool fieldFocused: gameExePath.activeFocus
                color: "transparent"

                Rectangle {anchors.fill: parent; radius: gameExePath.radius; color: "#3e3e3e"}

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 10; spacing: 5

                    IconImage {
                        source: "qrc:/qt/qml/vsmm/assets/icons/gamepad.svg"
                        color: gameExePath.fieldFocused ? "#ffffff" : "#888888"

                        Behavior on color {ColorAnimation {duration: 250}}
                    }

                    TextField
                    {
                        id: dupa
                        Layout.fillWidth: true
                        placeholderText: "Provide path to Vintage Story executable..."
                        color: "white"; font.pixelSize: 14

                        background: Rectangle {anchors.fill: parent; radius: gameExePath.radius; color: "#3e3e3e"}

                        Keys.onEscapePressed: (event) => {
                            focus = false
                            event.accepted = true
                        }
                    }
                }
            }

            ActionButton {
                buttonIcon: "folder.svg"
                display: AbstractButton.IconOnly
                Layout.preferredHeight: 40

                tooltipText: "Pick a folder"

                onClicked: {}
            }

        }

        RowLayout {
            Layout.fillWidth: true

            Label {text: "Current Path: "; font.pixelSize: 12; font.bold: true; color: "green"}
            TextEdit {
                text: Config.gameDir

                readOnly: true; selectByMouse: true; wrapMode: Text.WordWrap

                font.pixelSize: 12; color: "white"
                selectionColor: "lightblue"; selectedTextColor: "black"
            }
        }

        LayoutVerticalSpacer{}
    }
}