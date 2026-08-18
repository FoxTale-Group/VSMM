import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VSMM.Theme
import VSMM.Config
import VSMM.Templates
import VSMM.LayoutOrganization

TabPanel {
    id: settingsTabGeneral

    // Live switch positions.
    readonly property bool deleteOldModVersion: deleteModSwitch.checked
    readonly property bool includeModPrerelease: prereleaseSwitch.checked

    // Values currently in Config.
    readonly property bool savedDeleteOldModVersion: Config.general.deleteOldModVersion
    readonly property bool savedIncludeModPrerelease: Config.general.includeModPrerelease

    readonly property bool dirty: settingsTabGeneral.deleteOldModVersion !== settingsTabGeneral.savedDeleteOldModVersion
                               || settingsTabGeneral.includeModPrerelease !== settingsTabGeneral.savedIncludeModPrerelease

    // Assigns rather than binds: `checked` is user-writable, so a binding here would be
    // destroyed the first time the switch is clicked.
    function revert() {
        deleteModSwitch.checked = settingsTabGeneral.savedDeleteOldModVersion;
        prereleaseSwitch.checked = settingsTabGeneral.savedIncludeModPrerelease;
    }

    content: ColumnLayout {
        anchors.fill: parent
        spacing: 2

        RowLayout {
            LayoutHorizontalSpacer{}
            Label {
                text: qsTr("Mod Preferences:")
                font.pixelSize: Theme.fonts.title
                font.bold: true
            }
            LayoutHorizontalSpacer{}
            Layout.bottomMargin: 10
        }

        // --- Updates ------------------------------------------------------------

        RowLayout {
            LayoutHorizontalSpacer{}
            Label {
                text: qsTr("Updates")
                font.pixelSize: Theme.fonts.label
                font.bold: true
            }
            LayoutHorizontalSpacer{}
            Layout.bottomMargin: 10
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            ColumnLayout {
                Label {
                    text: qsTr("Include pre-release versions")
                    font.pixelSize: Theme.fonts.label
                }
                Label {
                    text: qsTr("Offer pre-release builds when checking for updates. These can be unstable.")

                    Layout.fillWidth: true
                    Layout.preferredWidth: 0

                    font.pixelSize: Theme.fonts.body
                    color: Theme.colors.labelAlt
                    wrapMode: Text.WordWrap
                }
            }
            
            LayoutHorizontalSpacer{}

            Switch { id: prereleaseSwitch }
        }

        LayoutHorizontalDivider{}

        // --- Manual installs ----------------------------------------------------

        RowLayout {
            LayoutHorizontalSpacer{}
            Label {
                text: qsTr("Manual installs")
                font.pixelSize: Theme.fonts.label
                font.bold: true
            }
            LayoutHorizontalSpacer{}
            Layout.bottomMargin: 10
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            ColumnLayout {
                Label {
                    text: qsTr("Replace older version on manual install")
                    font.pixelSize: Theme.fonts.label
                }
                Label {
                    text: qsTr("Updates always replace the old file. Turn this off to keep both versions in your mods folder when installing manually.")

                    Layout.fillWidth: true
                    Layout.preferredWidth: 0

                    font.pixelSize: Theme.fonts.body
                    color: Theme.colors.labelAlt
                    wrapMode: Text.WordWrap
                }
            }
            
            LayoutHorizontalSpacer{}

            Switch { id: deleteModSwitch }
        }

        LayoutVerticalSpacer{}
    }
}
