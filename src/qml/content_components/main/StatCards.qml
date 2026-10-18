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
                label: qsTr("Installed"),
                value: ModStore.installedModsCount,
                color: Theme.colors.statCardInstalledBg, textColor: Theme.colors.statCardInstalledText
            }, {
                label: qsTr("Updates available"),
                value: ModStore.updatesCount,
                color: Theme.colors.statCardUpdatesBg, textColor: Theme.colors.statCardUpdatesText
            }]

        delegate: Rectangle
        {
            required property var modelData

            Layout.fillWidth: true
            Layout.maximumWidth: 150
            Layout.preferredHeight: 64
            radius: Theme.radius.card
            color: modelData.color

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 2

                Label {
                    text: modelData.label
                    font.pixelSize: Theme.fonts.body
                    color: Theme.colors.labelAlt
                }
                Label {
                    text: modelData.value
                    font.pixelSize: Theme.fonts.headline
                    color: modelData.textColor
                }
            }
        }
    }
}