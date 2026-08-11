import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Effects
import vsmm

Rectangle {
    id: modEntryRoot
    width: modListView.width
    height: 64
    color: "transparent"

    HoverHandler {id: modEntryHoverHandler}

    // Divider between every mod entry
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
            font.pixelSize: Theme.fonts.body
            Layout.topMargin: 16
            Layout.bottomMargin: 16
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            padding: 0

            onCheckedChanged: {
                if (selectForUpdate.checked) {
                    console.log(modName + " is selected for update")
                } else {
                    console.log(modName + " is not selected for update anymore")
                }
                ModStore.markForUpdate(modId, checked)
            }
        }

        ModEntry_Icon {}

        ColumnLayout {
            spacing: 4
            clip: true

            // First Row: Mod info
            RowLayout {
                spacing: 6

                // Mod Name
                Label {
                    text: modName
                    color: Theme.colors.label
                    font.pixelSize: Theme.fonts.label
                    Layout.maximumWidth: 200
                    elide: Text.ElideRight
                }

                // Mod Author
                Label {
                    text: qsTr("by %1").arg(modAuthor)
                    font.pixelSize: Theme.fonts.body
                    color: Theme.colors.labelAlt
                }

                // Dot divider
                Label {
                    text: "·"
                    font.pixelSize: Theme.fonts.label
                    color: Theme.colors.labelAlt
                }

                // Mod version
                Label {
                    text: qsTr("v%1").arg(modVersion)
                    font.pixelSize: Theme.fonts.body
                    color: Theme.colors.labelVersion
                }

                // Update available badge
                Rectangle {
                    visible: modHasUpdate
                    radius: Theme.radius.badge
                    color: Theme.colors.modUpdateBadgeBg
                    implicitWidth: updateLabel.width + 16
                    implicitHeight: 18

                    Label {
                        id: updateLabel
                        anchors.centerIn: parent
                        text: qsTr("v%1 available").arg(modLatestVersion)
                        font.pixelSize: Theme.fonts.body
                        color: Theme.colors.modUpdateBadgeText
                    }
                }
                // Latest version badge
                Rectangle {
                    visible: !modHasUpdate
                    radius: Theme.radius.badge
                    color: Theme.colors.modLatestBadgeBg
                    implicitWidth: latestLabel.width + 16
                    implicitHeight: 18

                    Label {
                        id: latestLabel
                        anchors.centerIn: parent
                        text: qsTr("Latest")
                        font.pixelSize: Theme.fonts.body
                        color: Theme.colors.modLatestBadgeText
                    }
                }
            }

            // Second Row: Mod Tags
            RowLayout {
                Layout.maximumWidth: modEntryRoot.width * 0.6 // Restricted with of tags section, so mod entry buttons fit within window
                spacing: 8
                clip: true

                Repeater {
                    model: modTags
                    delegate: Label {
                        text: modelData
                        font.pixelSize: Theme.fonts.meta
                        color: Theme.colors.modTagText
                        background: Rectangle {
                            color: Theme.colors.modTagBg
                            radius: Theme.radius.chip
                        }
                        padding: 2
                    }
                }
                LayoutHorizontalSpacer{}
            }
        }
        LayoutHorizontalSpacer{}

        ModEntry_ButtonsSection{}
    }
}