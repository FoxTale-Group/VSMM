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

    HoverHandler {id: rowHoverHandler}

    // Divider
    HorizontalDivider {
        anchors.leftMargin: 6; anchors.rightMargin: 16
        visible: index !== modListView.count - 1
    }

    RowLayout {
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.leftMargin: 12; anchors.rightMargin: 12
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
            border.color: Theme.colors.modIconBorder

            property string coverUrl: modicon

            Rectangle {
                id: background
                anchors.fill: parent
                radius: modIcon.radius
                visible: true
                layer.enabled: true
                color: {
                    if (mainImage.status === Image.Ready) {
                        return Theme.colors.modIconBg
                    } else {return Theme.colors.modIconBgDefault}
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
                source: Theme.icons.iExtension
                color: Theme.colors.icon
                anchors.fill: parent; anchors.margins: 4
                sourceSize.width: modIcon.width; sourceSize.height: modIcon.height

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

            RowLayout {
                spacing: 6

                // Mod Name
                Label {
                    text: name
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: Theme.colors.label
                    Layout.maximumWidth: 200
                    elide: Text.ElideRight
                }

                // Mod Author
                Label {
                    text: "by " + author
                    font.pixelSize: 12
                    color: Theme.colors.labelAlt
                }

                // Dot divider
                Label {
                    text: "·"
                    font.pixelSize: 12
                    color: Theme.colors.labelAlt
                }

                // Mod version
                Label {
                    text: "v" + version
                    font.pixelSize: 11
                    color: Theme.colors.labelVersion
                }

                // Update available badge
                Rectangle {
                    visible: hasUpdate
                    radius: 6
                    color: Theme.colors.modUpdateBadgeBg
                    implicitWidth: updateLabel.width + 16
                    implicitHeight: 18

                    Label {
                        id: updateLabel
                        anchors.centerIn: parent
                        text: "v" + latestVersion + " available"
                        font.pixelSize: 11
                        color: Theme.colors.modUpdateBadgeText
                    }
                }
                // Latest version badge
                Rectangle {
                    visible: !hasUpdate
                    radius: 6
                    color: Theme.colors.modLatestBadgeBg
                    implicitWidth: latestLabel.width + 16
                    implicitHeight: 18

                    Label {
                        id: latestLabel
                        anchors.centerIn: parent
                        text: "Latest"
                        font.pixelSize: 11
                        color: Theme.colors.modLatestBadgeText
                    }
                }
            }

            // Tags
            RowLayout {
                Layout.maximumWidth: modEntry.width * 0.7
                spacing: 8
                clip: true

                Repeater {
                    model: tags
                    delegate: Label {
                        text: modelData
                        font.pixelSize: 10
                        color: Theme.colors.modTagText
                        background: Rectangle {
                            color: Theme.colors.modTagBg
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

            VsmmModEntryButton {
                icon.source: Theme.icons.iDownloadOne

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: hasUpdate ? "Download update for '" + name + "'" : ""

                defaultColor: "transparent"
                hoverColor: hasUpdate ? Theme.colors.buttonUpdateHover : "transparent"
                pressColor: hasUpdate ? Theme.colors.buttonUpdatePress : "transparent"

                iconColor: hasUpdate ? Theme.colors.buttonUpdateLabel : Theme.colors.labelAlt

                onClicked: {
                    if (hasUpdate) {
                        console.log("Updating mod" + name)
                    }
                }
            }

            VsmmModEntryButton {
                icon.source: Theme.icons.iCheckUpdate

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: "Check update for '" + name + "'"

                onClicked: {console.log("Checking update for " + name)}
            }

            VsmmModEntryButton {
                property bool favorited: false

                icon.source: favorited ? Theme.icons.iFavoriteFilled : Theme.icons.iFavorite
                iconColor: favorited ? Theme.colors.modFavButton : Theme.colors.icon

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: "Add '" + name + "' to favorites"

                onClicked: {
                    favorited = !favorited
                    console.log("Added '" + name + "' to favorites")
                }
            }

            VsmmModEntryButton {
                icon.source: Theme.icons.iOpenLink

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: "Open '" + name + "' mod page"

                onClicked: {
                    console.log("Opening " + url + " modpage")
                    Qt.openUrlExternally(url)
                }
            }

            VsmmModEntryButton {
                icon.source: Theme.icons.iDelete
                iconColor: Theme.colors.modDelButtonIcon

                Layout.leftMargin: 5

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: "Delete mod  '" + name +"'"

                hoverColor: Theme.colors.modDelButtonHover
                pressColor: Theme.colors.modDelButtonPress

                onClicked: {console.log("Deleting " + name)}
            }
        }
    }
}