import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Effects
import vsmm

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

    HoverHandler {
        id: rowHoverHandler
    }

    // Divider
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 6
        anchors.rightMargin: 16
        height: 1
        color: "#333333"
        visible: index !== modListView.count - 1
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
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            padding: 0

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
            id: modIcon
            Layout.topMargin: 12
            Layout.bottomMargin: 12
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            implicitWidth: 40
            implicitHeight: 40
            radius: 8
            border.width: 1
            border.color: "#000000"

            property string coverUrl: modicon

            Rectangle {
                id: background
                anchors.fill: parent
                radius: modIcon.radius
                visible: true
                layer.enabled: true
                color: {
                    if (mainImage.status === Image.Ready) {
                        return "#ffffff"
                    } else {return "#1D9E75"}
                }
            }

            Rectangle {
                id: maskTemplate
                anchors.fill: parent
                radius: modIcon.radius
                visible: false
                layer.enabled: true
            }

            IconImage {
                id: fallbackIcon
                source: "qrc:/qt/qml/vsmm/assets/icons/extension.svg"
                color: "white"
                anchors.fill: parent
                anchors.margins: 4
                sourceSize.width: modIcon.width
                sourceSize.height: modIcon.height

                visible: mainImage.status !== Image.Ready
            }

            Image {
                id: mainImage
                source: modIcon.coverUrl

                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop

                asynchronous: true

                sourceSize.width: modIcon.width
                sourceSize.height: modIcon.height

                layer.enabled: true
                layer.effect: MultiEffect {
                    maskEnabled: true
                    maskSource: maskTemplate
                }

                visible: status === Image.Ready
            }
        }

        ColumnLayout {
            spacing: 4
            clip: true

            // Mod name, version layout
            RowLayout {
                spacing: 6

                Label {
                    text: name
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: "#e0e0e0"
                    Layout.maximumWidth: 200
                    elide: Text.ElideRight
                }

                Label {
                    text: "by " + author
                    font.pixelSize: 12
                    color: "#999999"
                }

                Label {
                    text: "·"
                    font.pixelSize: 12
                    color: "#999999"
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
                        text: "Latest"
                        font.pixelSize: 11
                        color: "#00ff00"
                    }
                }
            }

            // Mod author, category layout
            RowLayout {
                Layout.maximumWidth: modEntry.width * 0.7
                spacing: 8
                clip: true

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

        // Mod Action Buttons
        RowLayout {
            spacing: 2
            clip: true

            opacity: rowHoverHandler.hovered ? 1.0 : 0.0
            visible: opacity > 0

            Behavior on opacity {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }

            ModActionButton {
                buttonIcon: "download_one.svg"

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: hasUpdate ? "Download update for '" + name + "'" : ""

                defaultColor: "transparent"
                hoverColor: hasUpdate ? "#423710" : "transparent"
                pressColor: hasUpdate ? "#231c07" : "transparent"

                iconColor: hasUpdate ? "#b5951c" : "#444444"

                onClicked: {
                    if (hasUpdate) {
                        console.log("Updating mod" + name)
                    }
                }
            }

            ModActionButton {
                buttonIcon: "check_update.svg"

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: "Check update for '" + name + "'"

                defaultColor: "transparent"
                hoverColor: "#444444"
                pressColor: "#333333"

                onClicked: {console.log("Checking update for " + name)}
            }

            ModActionButton {

                property string fav_icon: "favorite.svg"
                property string fav_icon_fill: "favorite_filled.svg"

                property bool favorited: false

                buttonIcon: favorited ? fav_icon_fill : fav_icon
                iconColor: favorited ? "#ca22c7" : "white"

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: "Add '" + name + "' to favorites"

                defaultColor: "transparent"
                hoverColor: "#444444"
                pressColor: "#333333"

                onClicked: {
                    favorited = !favorited
                    console.log("Added '" + name + "' to favorited")
                }
            }

            ModActionButton {
                buttonIcon: "open_link.svg"

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: "Open '" + name + "' mod page"

                defaultColor: "transparent"
                hoverColor: "#444444"
                pressColor: "#333333"

                onClicked: {
                    console.log("Opening " + url + " modpage")
                    Qt.openUrlExternally(url)
                }
            }

            ModActionButton {
                buttonIcon: "delete.svg"

                Layout.leftMargin: 5

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: "Delete mod  '" + name +"'"

                defaultColor: "transparent"
                hoverColor: "#572525"
                pressColor: "#291313"
                iconColor: "#dd1919"

                onClicked: {console.log("Deleting " + name)}
            }
        }
    }
}