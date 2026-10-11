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

        Switch { 
            id: enableMod
            trackWidth: 34
            trackHeight: 20
            thumbSize: 14
            thumbInset: 3
            ringWidth: 4

            // TODO: Wire with backend API
            checked: true // Placeholder until backend is wired
        }

        ModEntry_Icon {
            modEnabled: enableMod.checked
            onToggled: ModStore.markForUpdate(modId, checked)
        }

        ColumnLayout {
            spacing: 4
            clip: true
            opacity: enableMod.checked ? 1.0 : 0.5

            Behavior on opacity { NumberAnimation { duration: enableMod.animationDuration } }

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
                Layout.maximumWidth: modEntryRoot.width * 0.55 // Restricted width of tags section, so mod entry buttons fit within window
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