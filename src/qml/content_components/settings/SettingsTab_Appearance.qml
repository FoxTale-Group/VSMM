import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm
import VSMMStyle

TabPanel {
    id: settingsTabAppearance

    // Live selection. The swatches write straight to Theme so the whole app previews the
    // accent; this reads it back for the dirty comparison.
    readonly property int accentIndex: Theme.accentIndex
    
    // Values currently in Config.
    readonly property int savedAccentIndex: Config.appearance.accentIndex
    readonly property bool dirty: settingsTabAppearance.accentIndex !== settingsTabAppearance.savedAccentIndex

    function revert() {
        const i = Math.min(Math.max(settingsTabAppearance.savedAccentIndex, 0), Theme.colors.accents.length - 1);
        Theme.accentIndex = i;
        accentRepeater.itemAt(i).checked = true;
    }

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

            // Wraps to a new line once the swatches no longer fit, so the palette can grow
            // without pushing the row past the window edge. Its own tight spacing keeps the
            // swatches grouped independently of the outer row's.
            Flow {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 2

                // autoExclusive groups by parent, so sharing this Flow is what makes the
                // swatches mutually exclusive; no ButtonGroup is involved.
                Repeater {
                    id: accentRepeater
                    model: Theme.colors.accents

                    RadioButton {
                        required property int index
                        required property color modelData

                        swatchColor: modelData
                        onClicked: Theme.accentIndex = index
                    }
                }
            }
        }

        LayoutVerticalSpacer{}
    }
}