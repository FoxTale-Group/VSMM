pragma Singleton
import QtQml
import QtQuick

QtObject {
    id: _ThemeRoot

    readonly property string appLogo: "qrc:/qt/qml/vsmm/assets/logo/VSMM.png"

    property bool isDarkTheme: true

    component IconSet: QtObject
    {
        readonly property string _iconsPath: "qrc:/qt/qml/vsmm/assets/icons/"

        readonly property string addIcon:            _iconsPath + "add"              + ".svg"
        readonly property string addBoxIcon:         _iconsPath + "add_box"          + ".svg"
        readonly property string checkUpdateIcon:    _iconsPath + "check_update"     + ".svg"
        readonly property string closeIcon:          _iconsPath + "close"            + ".svg"
        readonly property string deleteIcon:         _iconsPath + "delete"           + ".svg"
        readonly property string downloadIcon:       _iconsPath + "download"         + ".svg"
        readonly property string downloadAllIcon:    _iconsPath + "download_all"     + ".svg"
        readonly property string downloadOneIcon:    _iconsPath + "download_one"     + ".svg"
        readonly property string dropItemIcon:       _iconsPath + "drop_item"        + ".svg"
        readonly property string exitFullscreenIcon: _iconsPath + "exit_fullscreen"  + ".svg"
        readonly property string extensionIcon:      _iconsPath + "extension"        + ".svg"
        readonly property string favoriteIcon:       _iconsPath + "favorite"         + ".svg"
        readonly property string favoriteFilledIcon: _iconsPath + "favorite_filled"  + ".svg"
        readonly property string filterIcon:         _iconsPath + "filter"           + ".svg"
        readonly property string folderIcon:         _iconsPath + "folder"           + ".svg"
        readonly property string folderOpenIcon:     _iconsPath + "folder_open"      + ".svg"
        readonly property string gamepadIcon:        _iconsPath + "gamepad"          + ".svg"
        readonly property string goFullscreenIcon:   _iconsPath + "fullscreen"       + ".svg"
        readonly property string launchIcon:         _iconsPath + "launch"           + ".svg"
        readonly property string minimizeIcon:       _iconsPath + "minimize"         + ".svg"
        readonly property string openLinkIcon:       _iconsPath + "open_link"        + ".svg"
        readonly property string searchIcon:         _iconsPath + "search"           + ".svg"
        readonly property string selectCheckBoxIcon: _iconsPath + "select_check_box" + ".svg"
        readonly property string settingsIcon:       _iconsPath + "settings"         + ".svg"
        readonly property string syncIcon:           _iconsPath + "sync"             + ".svg"
        readonly property string upgradeOneIcon:     _iconsPath + "upgrade_one"      + ".svg"
    }
    readonly property IconSet icons: IconSet {}

    property real fontScale: 1.0          // seeded from system, overridable in Settings
    component FontSet: QtObject
    {
        readonly property int headline: Math.round(22 * _ThemeRoot.fontScale)
        readonly property int title:    Math.round(18 * _ThemeRoot.fontScale)
        readonly property int label:    Math.round(14 * _ThemeRoot.fontScale)
        readonly property int body:     Math.round(12 * _ThemeRoot.fontScale)
        readonly property int meta:     Math.round(11 * _ThemeRoot.fontScale)
    }
    readonly property FontSet fonts: FontSet {}

    component RadiusSet: QtObject
    {
        readonly property int window:    10  // window frame, tab panel
        readonly property int panel:     12  // large containers: mod list, drop zone
        readonly property int field:      8  // text inputs and their wrappers
        readonly property int card:       8  // stat cards, mod icon tiles
        readonly property int button:     6  // Button, TabButton, ToolTip
        readonly property int badge:      6  // version badges on a mod row
        readonly property int chip:       4  // mod tag chips
        readonly property int indicator:  4  // CheckBox box
    }
    readonly property RadiusSet radius: RadiusSet {}

    component ColorSet: QtObject
    {
        // Window colors
        readonly property color windowBackground: "#1e1e1e"
        readonly property color windowBorder:     "#555555"
        readonly property color windowTitle:      "#7e7e7e"

        // Default text labels and icons
        readonly property color icon:              "white"
        readonly property color text:              "white"
        readonly property color textSelection:     "lightblue"
        readonly property color textSelected:      "black"
        readonly property color textWarning:       "yellow"
        readonly property color textError:         "red"
        readonly property color link:            "#1da7d5"

        readonly property color label:            "white"
        readonly property color labelAlt:       "#999999"
        readonly property color labelVersion:   "#888888"
        readonly property color labelHighlight: "green"
        
        // Divider
        readonly property color divider: "#333333"

        // Window buttons
        readonly property color winButtonCloseHover:     "#e81123"
        readonly property color winButtonCloseText:      "#cccccc"
        readonly property color winButtonCloseTextHover: "white"

        // Scrollbar colors
        readonly property color scrollBarHandleDefault: "#444444"
        readonly property color scrollBarHandleHover:   "#555555"
        readonly property color scrollBarHandlePress:   "#666666"

        // TabBar colors
        readonly property color tabButtonInactive:       "#323232"
        readonly property color tabButtonHover:          "#424242"
        readonly property color tabPanelBackground:      "#262626"
        readonly property color tabPanelBorder:          "#3a3a3a"
        readonly property color tabButtonInactiveBorder: tabButtonInactive
        readonly property color tabButtonHoverBorder:    tabButtonHover

        // Default button colors
        readonly property color buttonInactive: "#323232"
        readonly property color buttonDefault: "#595959"
        readonly property color buttonHover:   "#3e3e3e"
        readonly property color buttonPress:   "#2a2a2a"
        readonly property color buttonIcon:    "#ffffff"
        readonly property color buttonText:    "#ffffff"

        // Default mod entry action button colors
        readonly property color modEntryButtonDefault: "transparent"
        readonly property color modEntryButtonHover:   "#444444"
        readonly property color modEntryButtonPress:   "#333333"

        // Highlighted button colors
        readonly property color highlightButtonDefault: "#237cb8"
        readonly property color highlightButtonHover:   "#1f5c87"
        readonly property color highlightButtonPress:   "#143f5c"

        // Launch Game button colors
        readonly property color buttonLaunchDefault: "#33914e"
        readonly property color buttonLaunchHover:   "#2a7840"
        readonly property color buttonLaunchPress:   "#1e572e"

        // Add Mod button colors
        readonly property color buttonAddModDefault: "#1D9E75"
        readonly property color buttonAddModHover:   "#188160"
        readonly property color buttonAddModPress:   "#0d5e44"

        // Add Mod button colors
        readonly property color buttonUpdateLabel:   "#e0a23a"
        readonly property color buttonUpdateDefault: "#a6842e"
        readonly property color buttonUpdateHover:   "#7c6220"
        readonly property color buttonUpdatePress:   "#453713"

        // Stat cards colors
        readonly property color statCardInstalledBg:    "#2a2a2a"
        readonly property color statCardInstalledText:  "#e0e0e0"
        readonly property color statCardUpdatesBg:      "#3a2f12"
        readonly property color statCardUpdatesText:    "#e0a23a"

        // Search bar colors
        readonly property color searchBarBg:        "#3e3e3e"
        readonly property color searchBarDefault:   "#888888"
        readonly property color searchBarIcon:      "#ffffff"

        // Mod list colors
        readonly property color modlistBg:      "#262626"
        readonly property color modlistBorder:  "#3a3a3a"

        // Mod entry colors
        readonly property color modTagText:         "#888780"
        readonly property color modTagBg:           "#333333"
        readonly property color modIconBorder:      "#1D9E75"
        readonly property color modIconBgDefault:   "#1D9E75"
        readonly property color modIconBg:          "#000000"

        // Mod entry update badges colors
        readonly property color modUpdateBadgeBg:   "#3a2f12"
        readonly property color modUpdateBadgeText: "#e0a23a"
        readonly property color modLatestBadgeBg:   "#05552f"
        readonly property color modLatestBadgeText: "#00ff00"

        // Mod entry action buttons colors
        readonly property color modFavButton:       "#ca22c7"
        readonly property color modDelButtonIcon:   "#dd1919"
        readonly property color modDelButtonHover:  "#572525"
        readonly property color modDelButtonPress:  "#291313"


        // Drag and Drop Area colors
        readonly property color dropAreaBg:     "#2b2b2b"
        readonly property color dropAreaDragBg: "#2a3d4d"
        readonly property color dropAreaBorder: "#555555"
        readonly property color dropAreaIcon:   "#aaaaaa"
        readonly property color dropAreaDragFg: "#4da6ff"

    }
    readonly property ColorSet colors: ColorSet {}

}