import QtQml
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Dialogs
import vsmm
import VSMMStyle
import "../../js/StringHelpers.js" as StrUtils

TabPanel {
    id: _SettingsTab_Paths

    property string gameConfigDir: ""
    property string gameExePath: ""

    signal settingsEdited(edited: bool)

    content: ColumnLayout {
        anchors.fill: parent; spacing: 5;

        ColumnLayout { // VS Data Folder Path Setting
            Label {text: qsTr("VintagestoryData folder location:"); font.pixelSize: Theme.fonts.label}

            Label {
                id: _DataFolderDescLabel

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
                    radius: Theme.radius.field
                    color: "transparent"

                    property bool fieldFocused: gameConfigDirInputField.activeFocus

                    // On focus the filled panel wipes in while the resting underline wipes out.
                    AnimRadialReveal {
                        anchors.fill: parent
                        isRevealed: gameConfigDirPath.fieldFocused

                        animationDuration: 200

                        Rectangle {
                            anchors.fill: parent
                            radius: gameConfigDirPath.radius
                            color: Theme.colors.searchBarBg
                        }
                    }

                    AnimRadialReveal {
                        anchors.fill: parent
                        isRevealed: !gameConfigDirPath.fieldFocused

                        animationDuration: 200

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 6
                            anchors.rightMargin: 16
                            anchors.bottomMargin: 2
                            height: 1

                            color: Theme.colors.searchBarDefault
                        }
                    }

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 10; spacing: 5

                        IconImage {
                            source: Theme.icons.extensionIcon
                            color: gameConfigDirPath.fieldFocused ? Theme.colors.searchBarIcon : Theme.colors.searchBarDefault

                            Behavior on color {ColorAnimation {duration: 250}}
                        }

                        TextField {
                            id: gameConfigDirInputField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Provide path to VintagestoryData folder...")
                            font.pixelSize: Theme.fonts.body

                            // The reveal layers above paint the field, so the control
                            // must not draw a background over them.
                            background: Item {}

                            Keys.onEscapePressed: (event) => {
                                focus = false
                                event.accepted = true
                            }

                            onTextChanged: {
                                _SettingsTab_Paths.gameConfigDir = text

                                if(_SettingsTab_Paths.gameConfigDir !== Config.paths.gameConfig &&
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

                    onAccepted: { gameConfigDirInputField.text = StrUtils.getCleanPath(vsDataFolderPicker.selectedFolder.toString()) }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Current Path: ")
                    font.pixelSize: Theme.fonts.body
                    font.bold: true; color: Theme.colors.labelHighlight;
                }
                TextEdit {
                    text: Config.paths.gameConfig ?? ""

                    readOnly: true; selectByMouse: true
                    wrapMode: Text.WordWrap

                    font.pixelSize: Theme.fonts.body; color: Theme.colors.text
                    selectionColor: Theme.colors.textSelection; selectedTextColor: Theme.colors.textSelected
                }
            }
        }

        LayoutHorizontalDivider{}

        ColumnLayout {
            Label {text: qsTr("Game executable path"); font.pixelSize: Theme.fonts.label}

            Label {
                id: _GameExeDescLabel

                text: qsTr("This is required for 'Launch Game' button to work")

                font.pixelSize: Theme.fonts.body
                wrapMode: Text.WordWrap
                color: Theme.colors.labelAlt; linkColor: Theme.colors.link
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 10

                Rectangle {
                    id: gameExePath
                    Layout.fillWidth: true; Layout.preferredHeight: 40; radius: Theme.radius.field

                    property bool fieldFocused: gameExePathField.activeFocus
                    color: "transparent"

                    // On focus the filled panel wipes in while the resting underline wipes out.
                    AnimRadialReveal {
                        anchors.fill: parent
                        isRevealed: gameExePath.fieldFocused

                        animationDuration: 200

                        Rectangle {
                            anchors.fill: parent
                            radius: gameExePath.radius
                            color: Theme.colors.searchBarBg
                        }
                    }

                    AnimRadialReveal {
                        anchors.fill: parent
                        isRevealed: !gameExePath.fieldFocused

                        animationDuration: 200

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 6
                            anchors.rightMargin: 16
                            anchors.bottomMargin: 2
                            height: 1

                            color: Theme.colors.searchBarDefault
                        }
                    }

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 10; spacing: 5

                        IconImage {
                            source: Theme.icons.gamepadIcon
                            color: gameExePath.fieldFocused ? Theme.colors.searchBarIcon : Theme.colors.searchBarDefault

                            Behavior on color {ColorAnimation {duration: 250}}
                        }

                        TextField
                        {
                            id: gameExePathField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Provide path to Vintage Story executable...")
                            font.pixelSize: Theme.fonts.body

                            // The reveal layers above paint the field, so the control
                            // must not draw a background over them.
                            background: Item {}

                            Keys.onEscapePressed: (event) => {
                                focus = false
                                event.accepted = true
                            }

                            onTextChanged: {
                                _SettingsTab_Paths.gameExePath = text

                                if(_SettingsTab_Paths.gameExePath !== Config.paths.gameExe &&
                                        !StrUtils.isNullOrWhitespace(_SettingsTab_Paths.gameExePath)) {
                                    _SettingsTab_Paths.settingsEdited(true)
                                } else {
                                    _SettingsTab_Paths.settingsEdited(false)
                                }
                            }
                        }
                    }
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

                    onAccepted: { gameExePathField.text = StrUtils.getCleanPath(vsExePicker.selectedFile.toString()) }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Current Path: ");
                    font.pixelSize: Theme.fonts.body
                    font.bold: true; color: Theme.colors.labelHighlight
                }
                TextEdit {
                    text: Config.paths.gameExe ?? ""

                    readOnly: true; selectByMouse: true; wrapMode: Text.WordWrap

                    font.pixelSize: Theme.fonts.body; color: Theme.colors.text
                    selectionColor: Theme.colors.textSelection; selectedTextColor: Theme.colors.textSelected
                }
            }
        }

        LayoutHorizontalDivider{}

        LayoutVerticalSpacer{}
    }

}