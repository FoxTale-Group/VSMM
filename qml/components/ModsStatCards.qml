import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmodchecker

RowLayout
{
    id: statCards
    Layout.fillWidth: true
    spacing: 12

    property int updatesAvailable: {
        let c = 0;
        for (let i = 0; i < ModListModel.count; i++)
            if (ModListModel.get(i).updateVersion !== "latest") c++
        return c
    }

    Repeater
    {
        model:
            [{
                label: "Installed",
                value: ModListModel.count,
                color: "#2a2a2a", textColor: "#e0e0e0"
            }, {
                label: "Updates available",
                value: updatesAvailable,
                color: "#3a2f12", textColor: "#e0a23a"
            }]

        delegate: Rectangle
        {
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