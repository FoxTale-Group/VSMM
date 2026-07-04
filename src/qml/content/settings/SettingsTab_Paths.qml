import QtQml
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Dialogs
import vsmm
import "../../js/StringHelpers.js" as StrUtils

VsmmTabPanel {
    id: _SettingsTab_Paths

    property string gameConfigDir: "none"

    content: ColumnLayout {
        anchors.fill: parent; spacing: 5;

        Label {text: qsTr("Game config folder"); font.pixelSize: 14; font.bold: true; color: Theme.colors.label}

        RowLayout {
            Layout.fillWidth: true; spacing: 10

            Rectangle {
                id: gameConfigDirPath
                Layout.fillWidth: true;
                Layout.preferredHeight: 40;
                radius: 8
                color: "transparent"

                property bool fieldFocused: gameConfigDirInputField.activeFocus

                Rectangle {anchors.fill: parent; radius: parent.radius; color: Theme.colors.searchBarBg}

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 10; spacing: 5

                    IconImage {
                        source: Theme.icons.iExtension
                        color: gameConfigDirInputField.fieldFocused ? Theme.colors.searchBarIcon : Theme.colors.searchBarDefault

                        Behavior on color {ColorAnimation {duration: 250}}
                    }

                    TextField {
                        id: gameConfigDirInputField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Provide path to Vintage Story config folder...")
                        color: Theme.colors.text; font.pixelSize: 14

                        background: Rectangle {anchors.fill: parent; radius: gameConfigDirPath.radius; color: "#3e3e3e"}

                        Keys.onEscapePressed: (event) => {
                            focus = false
                            event.accepted = true
                        }

                        onTextChanged: { _SettingsTab_Paths.gameConfigDir = text }
                    }
                }
            }

            VsmmButton {
                icon.source: Theme.icons.iFolder
                display: AbstractButton.IconOnly
                Layout.preferredHeight: 40

                tooltipText: qsTr("Pick a folder")

                onClicked: {systemFilePicker.open()}
            }

            FolderDialog {
                id: systemFilePicker
                title: qsTr("Select Vintage Story config folder")

                currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]

                onAccepted: { gameConfigDirInputField.text = StrUtils.getCleanPath(systemFilePicker.selectedFolder.toString()) }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Current Path: ")
                font.pixelSize: 12; font.bold: true; color: "green";
            }
            TextEdit {
                text: Config.config.vsmm.configGamePath

                readOnly: true; selectByMouse: true
                wrapMode: Text.WordWrap

                font.pixelSize: 12; color: Theme.colors.text
                selectionColor: "lightblue"; selectedTextColor: "black"
            }
        }

        Label {text: qsTr("Game executable path"); font.pixelSize: 14; font.bold: true; color: Theme.colors.label}

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
                        source: Theme.icons.iGamepad
                        color: gameExePath.fieldFocused ? "#ffffff" : "#888888"

                        Behavior on color {ColorAnimation {duration: 250}}
                    }

                    TextField
                    {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Provide path to Vintage Story executable...")
                        color: Theme.colors.text; font.pixelSize: 14

                        background: Rectangle {anchors.fill: parent; radius: gameExePath.radius; color: "#3e3e3e"}

                        Keys.onEscapePressed: (event) => {
                            focus = false
                            event.accepted = true
                        }
                    }
                }
            }

            VsmmButton {
                icon.source: Theme.icons.iFolder
                display: AbstractButton.IconOnly
                Layout.preferredHeight: 40

                tooltipText: qsTr("Pick a folder")

                onClicked: {}
            }

        }

        RowLayout {
            Layout.fillWidth: true

            Label {text: qsTr("Current Path: "); font.pixelSize: 12; font.bold: true; color: "green"}
            TextEdit {
                text: ""

                readOnly: true; selectByMouse: true; wrapMode: Text.WordWrap

                font.pixelSize: 12; color: Theme.colors.text
                selectionColor: "lightblue"; selectedTextColor: "black"
            }
        }

        LayoutVerticalSpacer{}
    }

}