import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm
import VSMMStyle

TabPanel {
    id: settingsTabAppearance

    // TODO: no `appearance` schema in Config yet, so the language, theme and accent
    // controls are previews only and this tab is never dirty.
    readonly property bool dirty: false

    function revert() {}

    content: ColumnLayout {
        anchors.fill: parent
        spacing: 5

        RowLayout {
            Layout.bottomMargin: 10
            LayoutHorizontalSpacer{}
            Label {
                text: qsTr("Look & Feel:")
                font.pixelSize: Theme.fonts.title
                font.bold: true
            }
            LayoutHorizontalSpacer{}
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            ColumnLayout { // left column
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: qsTr("Application Language")
                        font.pixelSize: Theme.fonts.label
                    }

                    LayoutHorizontalSpacer{}

                    ComboBox {
                        Layout.preferredWidth: 200

                        model: [
                            "English (Default)",
                            "German",
                            "French",
                            "Spanish",
                            "Portuguese (Brazilian)",
                            "Russian",
                            "Polish",
                            "Czech",
                            "Italian",
                            "Dutch",
                            "Turkish",
                            "Ukrainian",
                            "Hungarian",
                            "Finnish",
                            "Swedish",
                            "Norwegian",
                            "Danish",
                            "Chinese (Simplified)",
                            "Japanese",
                            "Korean",
                            "Thai"]
                    }
                }
            }

            ColumnLayout { // right column
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    Label {
                        text: qsTr("Application Theme")
                        font.pixelSize: Theme.fonts.label
                    }

                    LayoutHorizontalSpacer{}

                    ComboBox {
                        Layout.preferredWidth: 100
                        model: ["Dark", "Light"]
                    }
                }
            }
        }

        LayoutHorizontalDivider{}

        RowLayout {
            Label {
                text: qsTr("Accent color:")
                font.pixelSize: Theme.fonts.label
            }

            LayoutHorizontalSpacer {}

            // autoExclusive groups by parent, so sharing this RowLayout is what makes
            // the swatches mutually exclusive; no ButtonGroup is involved.
            RadioButton { swatchColor: Theme.colors.accent0; checked: true }
            RadioButton { swatchColor: Theme.colors.accent1 }
            RadioButton { swatchColor: Theme.colors.accent2 }
            RadioButton { swatchColor: Theme.colors.accent3 }
            RadioButton { swatchColor: Theme.colors.accent4 }
            RadioButton { swatchColor: Theme.colors.accent5 }

            LayoutHorizontalSpacer {}
        }

        LayoutVerticalSpacer{}
    }
}