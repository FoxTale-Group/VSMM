import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Dialogs
import vsmodchecker

Rectangle {
    id: addModWindowContent

    anchors.fill: parent
    anchors.margins: 20

    color: "transparent"

    signal modFileReceived(string filePath)

    ColumnLayout {
        anchors.fill: parent
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
                    source: "qrc:/qt/qml/vsmodchecker/icons/drop_item.svg"
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
                        // Only get first file
                        let rawUrl = drop.urls[0].toString()

                        if (rawUrl.endsWith(".zip")) {
                            let cleanFilePath = addModWindowContent.getCleanPath(rawUrl)
                            let fileName = addModWindowContent.getFileName(cleanFilePath)

                            console.log("Sending to C++: " + cleanFilePath)
                            console.log("Showing in UI: " + fileName)

                            // Fire your signal
                            addModWindowContent.modFileReceived(cleanFilePath)
                            drop.accept()
                            Window.window.close()

                        } else {
                            console.log("Error: Only .zip files are allowed!")
                        }
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

            nameFilters: ["Vintage Story Mods (*.zip)", "All Files (*)"]

            onAccepted: {
                addModWindowContent.processFile(systemFilePicker.selectedFile.toString())
            }
        }
    }

    function getCleanPath(rawUrl) {
        // Format paths for C++ friendly paths
        let cleanPath = rawUrl.replace(/^(file:\/{2})/, "");

        // Windows drive letter fix (e.g., /C:/Users/... -> C:/Users/...)
        if (cleanPath.startsWith("/") && cleanPath.charAt(2) === ":") {
            cleanPath = cleanPath.substring(1);
        }

        return cleanPath;
    }
    function getFileName(path) {
        // A regular expression that splits the string at either a
        // forward slash (Linux) or backward slash (Windows) just to be safe!
        return path.split(/[/\\]/).pop();
    }
}