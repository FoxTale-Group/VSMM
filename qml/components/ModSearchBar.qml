import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

RowLayout
{
    Layout.fillWidth: true
    spacing: 10

    TextField
    {
        id: searchField
        Layout.fillWidth: true
        placeholderText: "Search installed mods..."
        font.pixelSize: 14
    }
}