import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import vsmodchecker

Rectangle {
    id: modEntry
    width: modListView.width
    height: 64
    color: "transparent"

    function tagColor(tag)
    {
        switch (tag)
        {
            case "Farming": return "#1D9E75"
            case "Survival": return "#7F77DD"
            case "Food": return "#D85A30"
            case "Animals": return "#D4537E"
            default: return "#888780"
        }
    }

    // Divider
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#333333"
        visible: index !== ModListModel.count - 1
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 10

        CheckBox {
            id: selectForUpdate
            checked: false
            Layout.topMargin: 16
            Layout.bottomMargin: 16
            Layout.leftMargin: -10
            Layout.rightMargin: -10
            width: 15
            height: 15
            onCheckedChanged: {
                if (checked) {
                    console.log(name + " is selected for update")
                } else {
                    console.log(name + " is not selected for update anymore")
                }
            }
        }

        // Mod Icon
        Rectangle {
            id: icon
            Layout.topMargin: 12
            Layout.bottomMargin: 12
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            width: 40
            height: 40
            radius: 8
            color: "#1D9E75"

            IconImage {
                source: "qrc:/qt/qml/vsmodchecker/icons/extension.svg"
                color: "white"
                anchors.centerIn: parent
                sourceSize.width: 24
                sourceSize.height: 24
            }
        }

        ColumnLayout {
            spacing: 4
            clip: true

            // Mod name, version layout
            RowLayout {
                spacing: 8

                Label {
                    text: name
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
                    visible: hasUpdate
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
                    visible: !hasUpdate
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
            }

            // Mod author, category layout
            RowLayout {
                Layout.maximumWidth: modEntry.width * 0.5
                spacing: 8
                clip: true

                Label {
                    text: "by " + author
                    font.pixelSize: 12
                    color: "#999999"
                }

                Label {
                    text: " · "
                    font.pixelSize: 12
                    color: "#999999"
                }

                Repeater {
                    model: tags
                    delegate: Label {
                        text: modelData
                        font.pixelSize: 10
                        color: tagColor(modelData)
                        background: Rectangle {
                            color: "#333333"
                            radius: 4
                        }
                        padding: 2
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        Item { Layout.fillWidth: true }



        Button {
            id: updateModButton
            text: "Download update"
            visible: hasUpdate
            highlighted: hasUpdate
            onClicked: console.log("Open add mod dialog")
        }

        ToolButton {
            id: menuButton
            text: "\u22EE"
            onClicked: console.log("Mod options for " + name)
        }

    }
}