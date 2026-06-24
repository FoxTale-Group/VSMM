import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

Rectangle {
    id: searchBarBackground
    Layout.fillWidth: true
    /*Layout.leftMargin: 10
    Layout.rightMargin: 10
    Layout.topMargin: 0
    Layout.bottomMargin: -5*/
    //Layout.maximumWidth: parent.width * 0.5
    Layout.preferredHeight: 40
    radius: 8

    property bool searchBarFocused: false

    color: searchBarFocused ? "#3e3e3e" : "#2b2b2b"
    //color: "transparent"

    RowLayout
    {
        anchors.fill: parent
        anchors.leftMargin: 10
        spacing: 5

        IconImage {
            source: "qrc:/qt/qml/vsmodchecker/icons/search.svg"
            color: "#888888"
            //sourceSize: Qt.size(20, 20)
        }

        TextField
        {
            id: searchInput
            Layout.fillWidth: true
            placeholderText: "Search installed mods..."
            color: "white"
            font.pixelSize: 14

            /*background: Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 6
                anchors.rightMargin: 16
                height: 1
                color: "#888888"
                visible: true
            }*/
            background: Item{}

            onFocusChanged: {
                searchBarBackground.searchBarFocused = !searchBarBackground.searchBarFocused
            }

            onTextChanged: {
                ModSortFilterModel.filterText = searchInput.text
            }
        }
    }
}

