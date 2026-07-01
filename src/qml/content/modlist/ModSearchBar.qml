import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

Rectangle {
    id: searchBar
    Layout.fillWidth: true
    Layout.preferredHeight: 40
    radius: 8

    property bool searchBarFocused: searchInput.activeFocus

    color: "transparent"

    AnimRadialReveal {
        id: revealEffect
        anchors.fill: parent
        isRevealed: searchBar.searchBarFocused

        animationDuration: 200

        Rectangle {
            anchors.fill: parent
            radius: searchBar.radius
            color: "#3e3e3e"
        }
    }

    AnimRadialReveal {
        anchors.fill: parent
        isRevealed: !searchBar.searchBarFocused

        animationDuration: 200

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 6
            anchors.rightMargin: 16
            anchors.bottomMargin: 2
            height: 1

            color: "#888888"
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        spacing: 5

        IconImage {
            source: "qrc:/qt/qml/vsmm/assets/icons/search.svg"
            color: searchBar.searchBarFocused ? "#ffffff" : "#888888"

            Behavior on color {
                ColorAnimation { duration: 250 }
            }
        }

        TextField
        {
            id: searchInput
            Layout.fillWidth: true
            placeholderText: "Search installed mods..."
            color: "white"
            font.pixelSize: 14

            background: Item{}

            Keys.onEscapePressed: (event) => {
                focus = false
                event.accepted = true
            }

            onTextChanged: {
                ModSortFilterModel.filterText = searchInput.text
            }
        }
    }
}

