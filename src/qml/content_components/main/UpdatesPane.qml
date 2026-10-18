import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VSMMStyle
import vsmm

Pane {
    id: updatesPanelRoot

    function stateText(state) {
        switch (state) {
        case ModUpdateModel.Queued:      return qsTr("Queued")
        case ModUpdateModel.Downloading: return qsTr("Downloading")
        case ModUpdateModel.Installing:  return qsTr("Installing")
        case ModUpdateModel.Done:        return qsTr("Done")
        case ModUpdateModel.Failed:      return qsTr("Failed")
        }
        return ""
    }

    FontMetrics {
        id: badgeMetrics
        font.pixelSize: Theme.fonts.body
    }

    // Calculate widest state label in current language for width of the badge column
    readonly property real badgeColumnWidth: {
        badgeMetrics.font // qmllint disable confusing-expression-statement
        let widest = 0
        for (let state = ModUpdateModel.Queued; state <= ModUpdateModel.Failed; ++state)
            widest = Math.max(widest, badgeMetrics.advanceWidth(stateText(state)))
        return Math.ceil(widest) + 16 + 8
    }

    function areUpdates() {
        return ModUpdateModel.count > 0
    }

    property bool open: false
    function show() {open = true;}
    function hide() { open = false }

    readonly property bool updateRunning: ModUpdateModel.active
    onUpdateRunningChanged: if (updateRunning) show()

    Layout.fillWidth: true
    Layout.preferredHeight: open ? implicitHeight : 0
    visible: Layout.preferredHeight > 0
    clip: true

    Behavior on Layout.preferredHeight {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label { 
                Layout.fillWidth: true
                visible: !updatesPanelRoot.areUpdates()
                
                text: qsTr("No updates are queued")
                font.pixelSize: Theme.fonts.label
                font.italic: true;
            }
            ProgressBar {
                id: totalProgressBar
                Layout.fillWidth: true
                thickness: 22
                to: ModUpdateModel.count
                value: ModUpdateModel.finishedCount

                visible: updatesPanelRoot.areUpdates()

                Label { 
                    anchors.centerIn: parent
                    text: {
                        if(ModUpdateModel.active) {
                            qsTr("Updating (%1/%2)").arg(ModUpdateModel.finishedCount).arg(ModUpdateModel.count)
                        } else {
                            qsTr("Updated (%1/%2)").arg(ModUpdateModel.finishedCount).arg(ModUpdateModel.count)
                        }
                    }
                    font.pixelSize: Theme.fonts.label
                }
                
            }
            Button {
                icon.source: Theme.icons.clearAllIcon
                display: AbstractButton.IconOnly
                flat: true
                tooltipText: qsTr("Clear")
                enabled: !updatesPanelRoot.updateRunning
                visible: updatesPanelRoot.areUpdates()
                onClicked: ModUpdateModel.clear()
            }
            Button {
                icon.source: Theme.icons.closeIcon
                display: AbstractButton.IconOnly
                flat: true
                tooltipText: qsTr("Close")
                enabled: !updatesPanelRoot.updateRunning
                onClicked: updatesPanelRoot.hide()
            }
        }

        ListView {
            id: modUpdateList

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 180)
            clip: true; reuseItems: true; cacheBuffer: 400

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            model: ModUpdateModel 
            delegate: Item {
                id: updateEntry
                width: modUpdateList.width
                height: updateEntryRow.implicitHeight + 6

                HorizontalDivider {
                    anchors.leftMargin: 6; anchors.rightMargin: 16
                    visible: index !== modUpdateList.count - 1
                }

                RowLayout {
                    id: updateEntryRow
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 24 // room for the ScrollBar
                    spacing: 8

                    Label { 
                        text: modName
                        Layout.preferredWidth: Math.min(modUpdateList.width * 0.20, 240); 
                        elide: Text.ElideRight 
                        font.pixelSize: Theme.fonts.body
                    }

                    RowLayout {
                        spacing: 4
                        Layout.fillWidth: false
                        Layout.preferredWidth: 200

                        Label {
                            text: fromVersion
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideLeft
                        }
                        Label { text: "➜" }
                        Label {
                            text: toVersion
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            elide: Text.ElideRight
                        }
                    }
                    
                    Item {
                        Layout.preferredWidth: updatesPanelRoot.badgeColumnWidth
                        implicitHeight: statusBadge.implicitHeight

                        Rectangle {
                            id: statusBadge
                            anchors.centerIn: parent

                            radius: Theme.radius.badge
                            color: {
                                switch (updateState) {
                                    case ModUpdateModel.Queued:      return Theme.colors.statusBadges.queued
                                    case ModUpdateModel.Downloading: return Theme.colors.statusBadges.downloading
                                    case ModUpdateModel.Installing:  return Theme.colors.statusBadges.installing
                                    case ModUpdateModel.Done:        return Theme.colors.statusBadges.done
                                    case ModUpdateModel.Failed:      return Theme.colors.statusBadges.failed
                                }
                                return Theme.colors.modTagBg
                            }
                            implicitWidth: statusLabel.width + 16
                            implicitHeight: 18

                            Label {
                                id: statusLabel
                                anchors.centerIn: parent
                                text: updatesPanelRoot.stateText(updateState)
                                font.pixelSize: Theme.fonts.body
                                color: Theme.shade(parent.color, 0.5)
                            }
                        }
                    }
                    
                    ProgressBar {
                        visible: updateState !== ModUpdateModel.Failed
                        Layout.fillWidth: true
                        value: progress
                        indeterminate: progress == -1 || updateState == ModUpdateModel.Installing || updateState == ModUpdateModel.Queued
                    }

                    Label {
                        visible: updateState == ModUpdateModel.Failed
                        Layout.fillWidth: true
                        text: errorText
                        elide: Text.ElideRight
                    }

                }
            }
        }
    }
}