import QtQml
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import VSMM.Theme
import VSMM.Helpers
import VSMM.Config
import VSMM.Templates
import VSMM.LayoutOrganization
import VSMMStyle

TabPanel {
    id: settingsTabConfiguration

    // Live field contents.
    readonly property string gameConfigDir: gameConfigDirInputField.text
    readonly property string gameExePath: gameExePathField.text

    // Values currently in Config.
    readonly property string savedGameConfigDir: Config.paths.gameConfig
    readonly property string savedGameExePath: Config.paths.gameExe

    readonly property bool dirty: settingsTabConfiguration.gameConfigDir !== settingsTabConfiguration.savedGameConfigDir
                               || settingsTabConfiguration.gameExePath !== settingsTabConfiguration.savedGameExePath

    // Assigns rather than binds: `text` is user-writable, so a binding here would be
    // destroyed the first time the field is typed in.
    function revert() {
        gameConfigDirInputField.text = settingsTabConfiguration.savedGameConfigDir;
        gameExePathField.text = settingsTabConfiguration.savedGameExePath;
    }

    content: ColumnLayout {
        anchors.fill: parent; spacing: 5;

        ColumnLayout { // VS Data Folder Path Setting
            RowLayout {
                LayoutHorizontalSpacer{}
                Label {
                    text: qsTr("Paths Configuration:")
                    font.pixelSize: Theme.fonts.title
                    font.bold: true
                }
                LayoutHorizontalSpacer{}
            }

            Label {
                text: qsTr("VintagestoryData folder")
                font.pixelSize: Theme.fonts.label
                font.bold: true
            }

            Label {
                id: dataFolderDescLabel

                readonly property string _wikiLink: "https://wiki.vintagestory.at/VintagestoryData_folder"

                text: qsTr("VintagestoryData folder is directory where game configuration and mods are stored. You can read more about it on %1wiki page%2")
                      .arg("<a href=\"" + _wikiLink + "\">").arg("</a>.")

                font.pixelSize: Theme.fonts.body
                textFormat: Text.StyledText ;wrapMode: Text.WordWrap
                color: Theme.colors.labelAlt; linkColor: Theme.colors.link

                onLinkActivated: (link) => Qt.openUrlExternally(link)

                HoverHandler {
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
                ToolTip.visible: !StringHelpers.isNullOrWhitespace(dataFolderDescLabel.hoveredLink)
                ToolTip.text: qsTr("Open in web browser: %1").arg(_wikiLink)
                ToolTip.delay: 400
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 10

                TextField {
                    id: gameConfigDirInputField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    iconSource: Theme.icons.extensionIcon
                    placeholderText: qsTr("VintagestoryData folder path: ")
                }

                Button {
                    icon.source: Theme.icons.folderIcon
                    display: AbstractButton.IconOnly
                    Layout.preferredHeight: 40

                    tooltipText: qsTr("Open folder selection dialog")

                    onClicked: {vsDataFolderPicker.open()}
                }

                FolderDialog {
                    id: vsDataFolderPicker
                    title: qsTr("Select 'VintagestoryData' folder")

                    currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]

                    onAccepted: { gameConfigDirInputField.text = StringHelpers.getCleanPath(vsDataFolderPicker.selectedFolder.toString()) }
                }
            }
        }

        //LayoutHorizontalDivider{}

        ColumnLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("VintageStory executable")
                font.pixelSize: Theme.fonts.label
                font.bold: true
            }

            Label {
                id: gameExeDescLabel

                text: qsTr("This is required for VSMM to check installed game version so updater can check if mods are compatible with installed version and 'Launch Game' button to work")

                Layout.fillWidth: true
                Layout.preferredWidth: 0

                font.pixelSize: Theme.fonts.body
                wrapMode: Text.WordWrap
                color: Theme.colors.labelAlt; linkColor: Theme.colors.link
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 10

                TextField {
                    id: gameExePathField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    iconSource: Theme.icons.gamepadIcon
                    placeholderText: qsTr("Vintage Story executable path: ")
                }

                Button {
                    icon.source: Theme.icons.folderIcon
                    display: AbstractButton.IconOnly
                    Layout.preferredHeight: 40

                    tooltipText: qsTr("Open file selection dialog")

                    onClicked: {vsExePicker.open()}
                }

                FileDialog {
                    id: vsExePicker
                    title: qsTr("Select Vintage Story executable file")

                    // TODO: C++ Backend to determine Host OS and determine actual filters to return here - ("*.exe" for Windows and All Files for other)
                    nameFilters: [qsTr("All Files (*)")]

                    currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]

                    onAccepted: { gameExePathField.text = StringHelpers.getCleanPath(vsExePicker.selectedFile.toString()) }
                }
            }
        }

        //LayoutHorizontalDivider{}

        LayoutVerticalSpacer{}
    }

}