import QtQml
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Dialogs
import vsmm
import "../../js/StringHelpers.js" as StrUtils

VsmmTabPanel {
    id: _SettingsTab_Paths

    property string gameConfigDir: ""
    property string gameExePath: ""

    signal settingsEdited(edited: bool)

    content: ColumnLayout {
        anchors.fill: parent; spacing: 5;

        ColumnLayout { // VS Data Folder Path Setting
            Label {text: qsTr("VintagestoryData folder location:"); font.pixelSize: 14; color: Theme.colors.label}

            Label {
                id: _DataFolderDescLabel

                readonly property string _wikiLink: "https://wiki.vintagestory.at/VintagestoryData_folder"

                text: qsTr("VintagestoryData folder is directory where game configuration and mods are stored. You can read more about it on %1wiki page%2")
                      .arg("<a href=\"" + _wikiLink + "\">").arg("</a>.")

                textFormat: Text.StyledText ;wrapMode: Text.WordWrap
                font.pixelSize: 12; color: Theme.colors.labelAlt; linkColor: Theme.colors.link

                onLinkActivated: (link) => Qt.openUrlExternally(link)

                HoverHandler {
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
                ToolTip.visible: !StrUtils.isNullOrWhitespace(_DataFolderDescLabel.hoveredLink)
                ToolTip.text: qsTr("Open in web browser: %1").arg(_wikiLink)
                ToolTip.delay: 400
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 10

                Rectangle {
                    id: gameConfigDirPath
                    Layout.fillWidth: true;
                    Layout.preferredHeight: 40;
                    radius: 8
                    color: "transparent"

                    property bool fieldFocused: gameConfigDirInputField.activeFocus

                    Rectangle {anchors.fill: parent; radius: parent.radius; color: Theme.colors.searchBarBg}

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 10; spacing: 5

                        IconImage {
                            source: Theme.icons.iExtension
                            color: gameConfigDirInputField.fieldFocused ? Theme.colors.searchBarIcon : Theme.colors.searchBarDefault

                            Behavior on color {ColorAnimation {duration: 250}}
                        }

                        TextField {
                            id: gameConfigDirInputField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Provide path to VintagestoryData folder...")
                            color: Theme.colors.text; font.pixelSize: 14

                            background: Rectangle {anchors.fill: parent; radius: gameConfigDirPath.radius; color: "#3e3e3e"}

                            Keys.onEscapePressed: (event) => {
                                focus = false
                                event.accepted = true
                            }

                            onTextChanged: {
                                _SettingsTab_Paths.gameConfigDir = text

                                if(_SettingsTab_Paths.gameConfigDir !== Config.config.vsmm.configGamePath &&
                                        !StrUtils.isNullOrWhitespace(_SettingsTab_Paths.gameConfigDir))
                                {
                                    _SettingsTab_Paths.settingsEdited(true)
                                } else {
                                    _SettingsTab_Paths.settingsEdited(false)
                                }
                            }
                        }
                    }
                }

                VsmmButton {
                    icon.source: Theme.icons.iFolder
                    display: AbstractButton.IconOnly
                    Layout.preferredHeight: 40

                    tooltipText: qsTr("Open folder selection dialog")

                    onClicked: {vsDataFolderPicker.open()}
                }

                FolderDialog {
                    id: vsDataFolderPicker
                    title: qsTr("Select 'VintagestoryData' folder")

                    currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]

                    onAccepted: { gameConfigDirInputField.text = StrUtils.getCleanPath(vsDataFolderPicker.selectedFolder.toString()) }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Current Path: ")
                    font.pixelSize: 12; font.bold: true; color: Theme.colors.labelHighlight;
                }
                TextEdit {
                    text: Config.config.vsmm.configGamePath

                    readOnly: true; selectByMouse: true
                    wrapMode: Text.WordWrap

                    font.pixelSize: 12; color: Theme.colors.text
                    selectionColor: Theme.colors.textSelection; selectedTextColor: Theme.colors.textSelected
                }
            }
        }

        LayoutHorizontalDivider{}

        ColumnLayout {
            Label {text: qsTr("Game executable path"); font.pixelSize: 14; color: Theme.colors.label}

            Label {
                id: _GameExeDescLabel

                text: qsTr("This is required for 'Launch Game' button to work")

                wrapMode: Text.WordWrap
                font.pixelSize: 12; color: Theme.colors.labelAlt; linkColor: Theme.colors.link
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 10

                Rectangle {
                    id: gameExePath
                    Layout.fillWidth: true; Layout.preferredHeight: 40; radius: 8

                    property bool fieldFocused: gameExePath.activeFocus
                    color: "transparent"

                    Rectangle {anchors.fill: parent; radius: gameExePath.radius; color: "#3e3e3e"}

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 10; spacing: 5

                        IconImage {
                            source: Theme.icons.iGamepad
                            color: gameExePath.fieldFocused ? "#ffffff" : "#888888"

                            Behavior on color {ColorAnimation {duration: 250}}
                        }

                        TextField
                        {
                            id: gameExePathField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Provide path to Vintage Story executable...")
                            color: Theme.colors.text; font.pixelSize: 14

                            background: Rectangle {anchors.fill: parent; radius: gameExePath.radius; color: "#3e3e3e"}

                            Keys.onEscapePressed: (event) => {
                                focus = false
                                event.accepted = true
                            }

                            onTextChanged: {
                                _SettingsTab_Paths.gameExePath = text

                                if(_SettingsTab_Paths.gameExePath !== Config.config.vsmm.gameExe &&
                                        !StrUtils.isNullOrWhitespace(_SettingsTab_Paths.gameExePath)) {
                                    _SettingsTab_Paths.settingsEdited(true)
                                } else {
                                    _SettingsTab_Paths.settingsEdited(false)
                                }
                            }
                        }
                    }
                }

                VsmmButton {
                    icon.source: Theme.icons.iFolder
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

                    onAccepted: { gameExePathField.text = StrUtils.getCleanPath(vsExePicker.selectedFile.toString()) }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Current Path: ");
                    font.pixelSize: 12; font.bold: true; color: Theme.colors.labelHighlight
                }
                TextEdit {
                    text: Config.config.vsmm.gameExe

                    readOnly: true; selectByMouse: true; wrapMode: Text.WordWrap

                    font.pixelSize: 12; color: Theme.colors.text
                    selectionColor: Theme.colors.textSelection; selectedTextColor: Theme.colors.textSelected
                }
            }
        }

        LayoutHorizontalDivider{}

        LayoutVerticalSpacer{}
    }

}