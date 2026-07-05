import QtQml
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Dialogs
import vsmm
import "js/StringHelpers.js" as StrUtils

VsmmWindow {
    id: _addModWindow

    dialog: true
    resizable: false
    modality: Qt.ApplicationModal
    windowTitle: qsTr("Install mod")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Label {
            text: qsTr("Install New Mod")
            font.pixelSize: 24
            font.bold: true
            color: Theme.colors.label
        }

        // Drag and drop area
        Rectangle {
            id: dropZone
            Layout.fillWidth: true; Layout.fillHeight: true
            radius: 12; border.width: 2

            color: dropArea.containsDrag ? Theme.colors.dropAreaDragBg : Theme.colors.dropAreaBg
            border.color: dropArea.containsDrag ? Theme.colors.dropAreaDragFg : Theme.colors.dropAreaBorder


            ColumnLayout {
                anchors.centerIn: parent; spacing: 10

                IconImage {
                    source: Theme.icons.iDropItem
                    color: dropArea.containsDrag ? Theme.colors.dropAreaDragFg : Theme.colors.dropAreaIcon
                    sourceSize: Qt.size(64, 64)
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    id: dragAndDropLabel
                    text: qsTr("Drag & Drop .zip file here\n...or click to browse")
                    color: Theme.colors.label
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            DropArea {
                id: dropArea
                anchors.fill: parent

                onDropped: (drop) => {
                    if (drop.hasUrls) {
                        let rawUrl = drop.urls[0].toString()
                        _addModWindow.sendFile(rawUrl)
                    }
                }
            }

            // Open file picker dialog
            MouseArea {
                anchors.fill: parent
                onClicked: systemFilePicker.open()

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }

        VsmmButton {
            text: qsTr("Add")
            icon.source: ""

            display: AbstractButton.TextOnly
            Layout.preferredHeight: 30

            onClicked: {
                console.log("Settings apply")
            }
        }
    }

    FileDialog {
        id: systemFilePicker
        title: qsTr("Select Mod Archive")

        currentFolder: StandardPaths.writableLocation(StandardPaths.DownloadLocation)

        nameFilters: [qsTr("Vintage Story Mod Archive (*.zip)"), qsTr("All Files (*)")]

        onAccepted: {
            let rawUrl = systemFilePicker.selectedFile.toString()
            _addModWindow.sendFile(rawUrl)
        }
    }

    function sendFile(path) {
        if (path.endsWith(".zip")) {
            let fileName = StrUtils.getFileName(path)

            console.log("Sending to C++: " + path)
            console.log("Showing in UI: " + fileName)

            ModStore.load(path)
            addModWindow.close()

        } else {
            console.log("Error: Only .zip files are allowed!")
        }
    }
}