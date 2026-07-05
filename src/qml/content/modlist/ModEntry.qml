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
                ModStore.markForUpdate(modid, checked)
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
            color: Theme.colors.modIconBg

            property string coverUrl: modicon

            Rectangle {
                id: background
                anchors.fill: parent
                radius: modIcon.radius
                color: Theme.colors.modIconBgDefault
                visible: mainImage.status !== Image.Ready

                IconImage {
                    id: fallbackIcon
                    source: Theme.icons.iExtension
                    color: Theme.colors.icon
                    anchors.fill: parent
                    anchors.margins: 4
                    sourceSize.width: modIcon.width
                    sourceSize.height: modIcon.height
                    visible: mainImage.status !== Image.Ready
                }
            }

            // Rounded square mask shape
            Rectangle {
                id: maskTemplate
                anchors.fill: parent
                radius: modIcon.radius
                visible: false
                layer.enabled: true
            }

            // Mod icon from icon provider
            Image {
                id: mainImage
                source: modIcon.coverUrl
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                sourceSize.width: modIcon.width
                sourceSize.height: modIcon.height
                visible: false          // drawn through the effect below
                layer.enabled: true     // keeps its texture realized even while hidden
            }

            // LAYER 2: Icon cropped to the rounded mask, shown only when ready
            MultiEffect {
                anchors.fill: parent
                source: mainImage
                maskEnabled: true
                maskSource: maskTemplate
                visible: mainImage.status === Image.Ready
            }

            // LAYER 3: Icon image border
            Rectangle {
                color: "transparent"
                anchors.fill: parent
                implicitWidth: parent.width
                implicitHeight: parent.height
                radius: parent.radius * 625E-3 // Convert border radius to 62.5% of background radius
                border.width: 1
                border.color: Theme.colors.modIconBorder
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
                    text: qsTr("by %1").arg(author)
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
                    text: qsTr("v%1").arg(version)
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
                        text: qsTr("v%1 available").arg(latestVersion)
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
                        text: qsTr("Latest")
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

                tooltipText: hasUpdate ? qsTr("Download update for '%1'").arg(name) : ""

                defaultColor: "transparent"
                hoverColor: hasUpdate ? Theme.colors.buttonUpdateHover : "transparent"
                pressColor: hasUpdate ? Theme.colors.buttonUpdatePress : "transparent"

                iconColor: hasUpdate ? Theme.colors.buttonUpdateLabel : Theme.colors.labelAlt
                enabled: hasUpdate
                onClicked: {
                        console.log("Updating mod" + name)
                        ModStore.update(modid);
                }
            }

            VsmmModEntryButton {
                icon.source: Theme.icons.iCheckUpdate

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: qsTr("Check update for '%1'").arg(name)

                onClicked: {console.log("Checking update for " + name)}
            }

            VsmmModEntryButton {
                property bool favorited: false

                icon.source: favorited ? Theme.icons.iFavoriteFilled : Theme.icons.iFavorite
                iconColor: favorited ? Theme.colors.modFavButton : Theme.colors.icon

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: qsTr("Add '%1' to favorites").arg(name)

                onClicked: {
                    favorited = !favorited
                    console.log("Added '" + name + "' to favorites")
                }
            }

            VsmmModEntryButton {
                icon.source: Theme.icons.iOpenLink

                Layout.preferredHeight: 35
                Layout.preferredWidth: 35

                tooltipText: qsTr("Open '%1' mod page").arg(name)

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

                tooltipText: qsTr("Delete mod '%1'").arg(name)

                hoverColor: Theme.colors.modDelButtonHover
                pressColor: Theme.colors.modDelButtonPress

                onClicked: {console.log("Deleting " + name)}
            }
        }
    }
}