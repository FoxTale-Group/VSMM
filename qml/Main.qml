import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 720
    height: 640
    visible: true
    title: "Vintage Story Mod Manager"
    color: "#1e1e1e"

    ListModel {
        id: modModel
        objectName: "modModel"

        function appendEntry(entry) {
            append(entry)
        }
    }

    function tagColor(tag) {
        switch (tag) {
            case "Farming": return "#1D9E75"
            case "Survival": return "#7F77DD"
            case "Food": return "#D85A30"
            case "Animals": return "#D4537E"
            default: return "#888780"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // Search bar + actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search installed mods..."
                font.pixelSize: 14
            }

            Button {
                text: "Check updates"
                onClicked: console.log("Checking for updates...")
            }

            Button {
                text: "Add mod"
                highlighted: true
                onClicked: console.log("Open add mod dialog")
            }
        }

        // Stat cards
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Repeater {
                model: [
                    { label: "Installed", value: modModel.count, color: "#2a2a2a", textColor: "#e0e0e0" },
                    { label: "Updates available", value: (function () {
                            var c = 0
                            for (var i = 0; i < modModel.count; i++)
                                if (modModel.get(i).updateVersion !== "latest") c++
                            return c
                        })(), color: "#3a2f12", textColor: "#e0a23a" }
                ]

                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    radius: 8
                    color: modelData.color

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 2

                        Label {
                            text: modelData.label
                            font.pixelSize: 12
                            color: "#999999"
                        }
                        Label {
                            text: modelData.value
                            font.pixelSize: 22
                            font.weight: Font.Medium
                            color: modelData.textColor
                        }
                    }
                }
            }
        }

        // Mod list
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#262626"
            border.color: "#3a3a3a"
            border.width: 1
            clip: true

            ListView {
                id: modListView
                anchors.fill: parent
                anchors.margins: 1
                model: modModel
                spacing: 0
                clip: true

                delegate: Rectangle {
                    width: modListView.width
                    height: 64
                    color: "transparent"

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: "#333333"
                        visible: index !== modModel.count - 1
                    }

                    Rectangle {
                        id: icon
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        width: 36
                        height: 36
                        radius: 8
                        color: Qt.darker(tagColor(tag), 1.8)

                        Label {
                            anchors.centerIn: parent
                            text: "\u29C9"
                            color: tagColor(tag)
                            font.pixelSize: 16
                        }
                    }

                    ToolButton {
                        id: menuButton
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        text: "\u22EE"
                        onClicked: console.log("Mod options for " + modName)
                    }

                    Button {
                        id: updateModButton
                        text: "Download update"
                        visible: updateVersion !== "latest"
                        highlighted: updateVersion !== "latest"
                        anchors.right: menuButton.left
                        anchors.rightMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: console.log("Open add mod dialog")
                    }

                    ColumnLayout {
                        anchors.left: icon.right
                        anchors.leftMargin: 12
                        anchors.right: updateModButton.left
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2
                        clip: true

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: modName
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                color: "#e0e0e0"
                                Layout.maximumWidth: 200
                                elide: Text.ElideRight
                            }
                            Label {
                                text: "v" + version
                                font.pixelSize: 11
                                color: "#888888"
                            }
                            Rectangle {
                                visible: updateVersion !== "latest"
                                radius: 6
                                color: "#3a2f12"
                                implicitWidth: updateLabel.width + 16
                                implicitHeight: 18

                                Label {
                                    id: updateLabel
                                    anchors.centerIn: parent
                                    text: "v" + updateVersion + " available"
                                    font.pixelSize: 11
                                    color: "#e0a23a"
                                }
                            }
                            Rectangle {
                                visible: updateVersion === "latest"
                                radius: 6
                                color: "#05552f"
                                implicitWidth: latestLabel.width + 16
                                implicitHeight: 18

                                Label {
                                    id: latestLabel
                                    anchors.centerIn: parent
                                    text: "Latest version"
                                    font.pixelSize: 11
                                    color: "#00ff00"
                                }
                            }
                            Item { Layout.fillWidth: true } // absorbs leftover space, never the switch/button
                        }

                        Label {
                            text: "by " + author + " · " + tag
                            font.pixelSize: 12
                            color: "#999999"
                        }
                    }
                }
            }
        }
    }
}
