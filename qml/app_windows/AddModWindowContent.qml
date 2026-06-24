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
                        let fileUrl = drop.urls[0].toString()

                        if (fileUrl.endsWith(".zip")) {
                            addModWindowContent.processFile(fileUrl)
                            drop.accept()
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

    function processFile(rawUrl) {
        // Format paths for C++ friendly paths

        // Remove path prefix for Linux
        let cleanPath = rawUrl.replace(/^(file:\/{2})/, "");

        // Remove path prefix for Windows
        if (cleanPath.startsWith("/") && cleanPath.charAt(2) === ":") {
            cleanPath = cleanPath.substring(1);
        }

        console.log("Valid mod file ready for C++: " + cleanPath)

        // Fire the signal and close window
        addModWindowContent.modFileReceived(cleanPath)
        Window.window.close()
    }
}