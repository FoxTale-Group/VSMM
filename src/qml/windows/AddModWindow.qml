import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Dialogs
import vsmm

VsmmWindow {
    id: _addModWindow

    dialog: true
    resizable: false
    modality: Qt.ApplicationModal
    windowTitle: "Install mod"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Label {
            text: "Install New Mod"
            font.pixelSize: 24
            font.bold: true
            color: "white"
        }

        // Drag and drop area
        Rectangle {
            id: dropZone
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: dropArea.containsDrag ? "#2a3d4d" : "#2b2b2b"
            radius: 12
            border.color: dropArea.containsDrag ? "#4da6ff" : "#555555"
            border.width: 2

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 10

                IconImage {
                    source: "qrc:/qt/qml/vsmm/assets/icons/drop_item.svg"
                    color: dropArea.containsDrag ? "#4da6ff" : "#888888"
                    sourceSize: Qt.size(64, 64)
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    id: dragAndDropLabel
                    text: "Drag & Drop .zip file here\n...or click to browse"
                    color: dropArea.containsDrag ? "white" : "#aaaaaa"
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
            }
        }

        FileDialog {
            id: systemFilePicker
            title: "Select Mod Archive"

            currentFolder: StandardPaths.writableLocation(StandardPaths.DownloadsLocation)

            nameFilters: ["Vintage Story Mod Archive (*.zip)", "All Files (*)"]

            onAccepted: {
                let rawUrl = systemFilePicker.selectedFile.toString()
                _addModWindow.sendFile(rawUrl)
            }
        }
    }

    function sendFile(path) {
        if (path.endsWith(".zip")) {
            let fileName = _addModWindow.getFileName(path)

            console.log("Sending to C++: " + path)
            console.log("Showing in UI: " + fileName)

            ModStore.load(path)
            addModWindow.close()

        } else {
            console.log("Error: Only .zip files are allowed!")
        }
    }

    function getFileName(path) {
        let cleanPath = path.replace(/^(file:\/{2})/, "");

        if (cleanPath.startsWith("/") && cleanPath.charAt(2) === ":") {
            cleanPath = cleanPath.substring(1);
        }

        // A regular expression that splits the string at either a
        // forward slash (Linux) or backward slash (Windows) just to be safe!
        return cleanPath.split(/[/\\]/).pop();
    }
}