import QtQml
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Dialogs
import vsmm

VsmmWindow {
    id: addModWindow

    property string modPath: ""

    width: 400
    height: 300

    dialog: true
    resizable: false
    modality: Qt.ApplicationModal
    windowTitle: qsTr("Install mod")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 5

        Label {
            text: qsTr("Install New Mod")
            font.pixelSize: Theme.fonts.headline
            font.bold: true
        }

        // Drag and drop area
        Rectangle {
            id: dropZone
            Layout.fillWidth: true; Layout.fillHeight: true
            radius: Theme.radius.panel; border.width: 2

            color: dropArea.containsDrag ? Theme.colors.dropAreaDragBg : Theme.colors.dropAreaBg
            border.color: dropArea.containsDrag ? Theme.colors.dropAreaDragFg : Theme.colors.dropAreaBorder


            ColumnLayout {
                anchors.centerIn: parent; spacing: 10

                IconImage {
                    source: Theme.icons.dropItemIcon
                    color: dropArea.containsDrag ? Theme.colors.dropAreaDragFg : Theme.colors.dropAreaIcon
                    sourceSize: Qt.size(64, 64)
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    id: dragAndDropLabel
                    text: qsTr("Drag & Drop .zip file here\n...or click to browse")
                    font.pixelSize: Theme.fonts.body
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            DropArea {
                id: dropArea
                anchors.fill: parent

                onDropped: (drop) => {
                    if (drop.hasUrls) {
                        addModWindow.modPath = drop.urls[0].toString()
                        modPathLabel.text = qsTr("Selected mod: ") + StringHelpers.getFileName(addModWindow.modPath)
                        modPathLabel.color = Theme.colors.label
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

        Label {
            id: modPathLabel
            text: ""
            font.pixelSize: Theme.fonts.body
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.fillWidth: true;
            Layout.margins: 0
            spacing: 0

            LayoutHorizontalSpacer{}

            Button {
                text: qsTr("Add")
                icon.source: ""

                display: AbstractButton.TextOnly
                Layout.preferredHeight: 30

                onClicked: {
                    console.log("Add mod")
                    sendFile(addModWindow.modPath)
                }
            }
            LayoutHorizontalSpacer{}
        }

        Component.onCompleted: {
            addModWindow.modPath = ""
            modPathLabel.text = qsTr("Selected mod: ")
            modPathLabel.color = Theme.colors.label
        }

        Component.onDestruction: {
            addModWindow.modPath = ""
            modPathLabel.text = qsTr("Selected mod: ")
            modPathLabel.color = Theme.colors.label
        }
    }

    FileDialog {
        id: systemFilePicker
        title: qsTr("Select Mod Archive")

        currentFolder: StandardPaths.writableLocation(StandardPaths.DownloadLocation)

        nameFilters: [qsTr("Vintage Story Mod Archive (*.zip)"), qsTr("All Files (*)")]

        onAccepted: {
            addModWindow.modPath = systemFilePicker.selectedFile.toString()
            modPathLabel.text = qsTr("Selected mod: ") + StringHelpers.getFileName(addModWindow.modPath)
            modPathLabel.color = Theme.colors.label
        }
    }

    function sendFile(path) {
        if (path.endsWith(".zip")) {
            let fileName = StringHelpers.getFileName(path)

            console.log("Sending to C++: " + path)
            console.log("Showing in UI: " + fileName)

            ModStore.load(path)
            addModWindow.close()

        } else {
            if(StringHelpers.isNullOrWhitespace(path)){
                modPathLabel.text = qsTr("No file is selected")
                modPathLabel.color = Theme.colors.textWarning
            }
            modPathLabel.text = qsTr("Only .zip files are allowed!")
            modPathLabel.color = Theme.colors.textError
        }
    }
}