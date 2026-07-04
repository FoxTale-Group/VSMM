pragma Singleton
import QtQml
import QtQuick

QtObject {
    id: _ThemeRoot

    readonly property string appLogo: "qrc:/qt/qml/vsmm/assets/logo/VSMM.png"

    readonly property int windowRadius: 10
    readonly property int buttonCornerRadius: 6

    readonly property QtObject colors: QtObject
    {
        // Window colors
        readonly property color windowBackground: "#1e1e1e"
        readonly property color windowBorder:     "#555555"
        readonly property color windowTitle:      "#7e7e7e"

        // Default text labels and icons
        readonly property color icon: "white"
        readonly property color text: "white"

        readonly property color label: "white"
        readonly property color labelAlt: "#999999"
        readonly property color labelVersion: "#888888"

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

    readonly property QtObject icons: QtObject
    {
        readonly property string _iconsPath: "qrc:/qt/qml/vsmm/assets/icons/"

        readonly property string iAdd:              _iconsPath + "add"              + ".svg"
        readonly property string iAddBox:           _iconsPath + "add_box"          + ".svg"
        readonly property string iCheckUpdate:      _iconsPath + "check_update"     + ".svg"
        readonly property string iClose:            _iconsPath + "close"            + ".svg"
        readonly property string iDelete:           _iconsPath + "delete"           + ".svg"
        readonly property string iDownload:         _iconsPath + "download"         + ".svg"
        readonly property string iDownloadAll:      _iconsPath + "download_all"     + ".svg"
        readonly property string iDownloadOne:      _iconsPath + "download_one"     + ".svg"
        readonly property string iDropItem:         _iconsPath + "drop_item"        + ".svg"
        readonly property string iExitFullscreen:   _iconsPath + "exit_fullscreen"  + ".svg"
        readonly property string iExtension:        _iconsPath + "extension"        + ".svg"
        readonly property string iFavorite:         _iconsPath + "favorite"         + ".svg"
        readonly property string iFavoriteFilled:   _iconsPath + "favorite_filled"  + ".svg"
        readonly property string iFilter:           _iconsPath + "filter"           + ".svg"
        readonly property string iFolder:           _iconsPath + "folder"           + ".svg"
        readonly property string iFolderOpen:       _iconsPath + "folder_open"      + ".svg"
        readonly property string iGamepad:          _iconsPath + "gamepad"          + ".svg"
        readonly property string iGoFullscreen:     _iconsPath + "fullscreen"       + ".svg"
        readonly property string iLaunch:           _iconsPath + "launch"           + ".svg"
        readonly property string iMinimize:         _iconsPath + "minimize"         + ".svg"
        readonly property string iOpenLink:         _iconsPath + "open_link"        + ".svg"
        readonly property string iSearch:           _iconsPath + "search"           + ".svg"
        readonly property string iSelectCheckBox:   _iconsPath + "select_check_box" + ".svg"
        readonly property string iSettings:         _iconsPath + "settings"         + ".svg"
        readonly property string iSync:             _iconsPath + "sync"             + ".svg"
        readonly property string iUpgradeOne:       _iconsPath + "upgrade_one"      + ".svg"
    }
}