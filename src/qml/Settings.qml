import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm

VsmmWindow {
    id: _settingsWindow

    width: 800; height: 600; minimumWidth: 600; minimumHeight: 400;

    windowTitle: "Settings"; windowIcon: Theme.icons.iSettings;

    dialog: true; movable: false; resizable: false;
    modality: Qt.ApplicationModal

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20;

        TabBar {
            id: settingsTabBar
            Layout.fillWidth: true

            TabButton { text: "General"}
            TabButton { text: "Paths"}
            TabButton { text: "Appearance"}
            TabButton { text: "Advanced"}
        }

        StackLayout {
            id: settingsLayout

            Layout.fillWidth: true; Layout.fillHeight: true

            currentIndex: settingsTabBar.currentIndex

            SettingsTab_General{}
            SettingsTab_Paths{}
            SettingsTab_Appearance{}
            SettingsTab_Advanced{}
        }

        RowLayout {
            Layout.fillWidth: true;

            LayoutHorizontalSpacer{}

            VsmmButton {
                text: "Apply"
                icon.source: ""

                display: AbstractButton.TextOnly
                Layout.preferredHeight: 30

                onClicked: console.log("Settings apply")
            }

            VsmmButton {
                text: "Close"
                icon.source: ""

                display: AbstractButton.TextOnly
                Layout.preferredHeight: 30

                onClicked: console.log("settings close")
            }

            LayoutHorizontalSpacer{}
        }
    }
}