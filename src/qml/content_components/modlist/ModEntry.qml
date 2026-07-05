import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Effects
import vsmm

Rectangle {
    id: _modEntry
    width: modListView.width
    height: 64
    color: "transparent"

    HoverHandler {id: _ModEntryHoverHandler}

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
            id: _selectForUpdate
            checked: false
            Layout.topMargin: 16
            Layout.bottomMargin: 16
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            padding: 0

            onCheckedChanged: {
                // TODO: Implement selecting and saving mod to update queue
                if (_selectForUpdate.checked) {
                    console.log(name + " is selected for update")
                } else {
                    console.log(name + " is not selected for update anymore")
                }
                ModStore.markForUpdate(modid, checked)
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
                    implicitWidth: _updateLabel.width + 16
                    implicitHeight: 18

                    Label {
                        id: _updateLabel
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
                    implicitWidth: _latestLabel.width + 16
                    implicitHeight: 18

                    Label {
                        id: _latestLabel
                        anchors.centerIn: parent
                        text: qsTr("Latest")
                        font.pixelSize: 11
                        color: Theme.colors.modLatestBadgeText
                    }
                }
            }

            // Second Row: Mod Tags
            RowLayout {
                Layout.maximumWidth: _modEntry.width * 0.7
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
                LayoutHorizontalSpacer{}
            }
        }
        LayoutHorizontalSpacer{}

        ModEntry_ButtonsSection{}
    }
}