import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm

RowLayout
{
    id: statCards
    Layout.fillWidth: true
    spacing: 12

    Repeater
    {
        model:
            [{
                label: "Installed",
                value: ModStore.count,
                color: Theme.colors.statCardInstalledBg, textColor: Theme.colors.statCardInstalledText
            }, {
                label: "Updates available",
                value: ModStore.updates,
                color: Theme.colors.statCardUpdatesBg, textColor: Theme.colors.statCardUpdatesText
            }]

        delegate: Rectangle
        {
            Layout.fillWidth: true
            Layout.maximumWidth: 150
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