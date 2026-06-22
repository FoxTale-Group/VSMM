import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import main

ApplicationWindow
{
    id: windowMain

    Component.onCompleted: {
        console.log("name: ", Qt.application.name)
        console.log("version: ", Qt.application.version)
    }

    //region WINDOW SIZE
    width: 800
    height: 640
    minimumWidth: 750
    minimumHeight: 450
    //endregion

    //region WINDOW STYLE
    flags: Qt.Window | Qt.FramelessWindowHint
    visible: true
    color: "transparent"
    background: Rectangle {
        anchors.fill: parent
        color: "#1e1e1e" // Main app background color
        radius: {
            if (Window.window.visibility === Window.Maximized) {
                return 0
            } else {
                return 10 // Window corner radius
            }
        }

        // 3. Move the border here so it perfectly hugs the rounded corners
        border.color: "#555555"
        border.width: {
            if (Window.window.visibility === Window.Maximized) {
                return 0
            } else {
                return 1 // If window is not maximized, display border
            }
        }
    }
    //endregion

    //region RESIZE EDGES & CORNERS
    Item {
        parent: Overlay.overlay
        anchors.fill: parent

        // Top Edge
        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 5
            cursorShape: Qt.SizeVerCursor
            onPressed: Window.window.startSystemResize(Qt.TopEdge)
        }
        // Bottom Edge
        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 5
            cursorShape: Qt.SizeVerCursor
            onPressed: Window.window.startSystemResize(Qt.BottomEdge)
        }
        // Left Edge
        MouseArea {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 5
            cursorShape: Qt.SizeHorCursor
            onPressed: Window.window.startSystemResize(Qt.LeftEdge)
        }
        // Right Edge
        MouseArea {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 5
            cursorShape: Qt.SizeHorCursor
            onPressed: Window.window.startSystemResize(Qt.RightEdge)
        }
        // Top-Left Corner
        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            width: 8
            height: 8
            cursorShape: Qt.SizeFDiagCursor
            onPressed: Window.window.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
        }

        // Top-Right Corner
        MouseArea {
            anchors.top: parent.top
            anchors.right: parent.right
            width: 8
            height: 8
            cursorShape: Qt.SizeBDiagCursor
            onPressed: Window.window.startSystemResize(Qt.TopEdge | Qt.RightEdge)
        }

        // Bottom-Left Corner
        MouseArea {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 8
            height: 8
            cursorShape: Qt.SizeBDiagCursor
            onPressed: Window.window.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
        }

        // Bottom-Right Corner
        MouseArea {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 8
            height: 8
            cursorShape: Qt.SizeFDiagCursor
            onPressed: Window.window.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
        }
    }
    //endregion

    //region TITLE BAR
    header: Item {
        height: 30

        // A. The Main Title Bar Background (Rounds all 4 corners)
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: {
                if (Window.window.visibility === Window.Maximized) {
                    return 0
                } else {
                    return 10 // Must match the window radius!
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onPressed: Window.window.startSystemMove()
            // Double-click to maximize/restore
            onDoubleClicked: {
                if (Window.window.visibility === Window.Maximized) {
                    Window.window.showNormal()
                } else {
                    Window.window.showMaximized()
                }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.bottomMargin: 0
            anchors.topMargin: 3
            anchors.rightMargin: 3

            Text {
                id: windowTitleText
                text: Qt.application.name
                color: "white"
            }
            Text {
                id: versionTextDisplay
                text: "v" + Qt.application.version
                color: "#7e7e7e"
                Layout.fillWidth: true
            }

            // Minimize Button
            Button {
                icon.source: "qrc:/qt/qml/main/icons/minimize.svg"
                icon.color: "white"
                display: AbstractButton.IconOnly
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30


                background: Rectangle {
                    color: parent.hovered ? "#444444" : "transparent"
                    radius: 8
                }

                onClicked: Window.window.showMinimized()
            }

            // Maximize / Restore Button
            Button {
                // Change icon based on current state
                icon.source: Window.window.visibility === Window.Maximized ? "qrc:/qt/qml/main/icons/close_fullscreen.svg" : "qrc:/qt/qml/main/icons/fullscreen.svg"
                icon.color: "white"
                display: AbstractButton.IconOnly
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30

                background: Rectangle {
                    color: parent.hovered ? "#444444" : "transparent"
                    radius: 8
                }

                onClicked: {
                    if (Window.window.visibility === Window.Maximized) {
                        Window.window.showNormal()
                    } else {
                        Window.window.showMaximized()
                    }
                }
            }

            // Close Button
            Button {
                icon.source: "qrc:/qt/qml/main/icons/close.svg"
                icon.color: "white"
                display: AbstractButton.IconOnly
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30

                // Standard convention: Close buttons turn red on hover
                background: Rectangle {
                    color: parent.hovered ? "#e81123" : "transparent"
                    radius: 8
                }
                palette.buttonText: parent.hovered ? "white" : "#cccccc"

                onClicked: Window.window.close()
            }
        }
    }
    //endregion

    property int updatesAvailable: {
        let c = 0;
        for (let i = 0; i < ModListModel.count; i++)
            if (ModListModel.get(i).updateVersion !== "latest") c++
        return c
    }

    function tagColor(tag)
    {
        switch (tag)
        {
            case "Farming": return "#1D9E75"
            case "Survival": return "#7F77DD"
            case "Food": return "#D85A30"
            case "Animals": return "#D4537E"
            default: return "#888780"
        }
    }

    //region APP CONTENT
    ColumnLayout
    {
        id: appContent
        anchors.fill: parent
        anchors.topMargin: 7
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        spacing: 16

        //region STATUS CARDS
        RowLayout
        {
            id: statCards
            Layout.fillWidth: true
            spacing: 12

            Repeater
            {
                model:
                    [{
                        label: "Installed",
                        value: ModListModel.count,
                        color: "#2a2a2a", textColor: "#e0e0e0"
                    }, {
                        label: "Updates available",
                        value: windowMain.updatesAvailable,
                        color: "#3a2f12", textColor: "#e0a23a"
                    }]

                delegate: Rectangle
                {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    radius: 8
                    color: modelData.color

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 2

                        Label {
                            text: modelData.label
                            font.pixelSize: 12
                            color: "#999999"
                        }
                        Label {
                            text: modelData.value
                            font.pixelSize: 22
                            font.weight: Font.Medium
                            color: modelData.textColor
                        }
                    }
                }
            }
        }
        //endregion

        //region BUTTONS
        RowLayout
        {
            id: actionButtons
            Layout.fillWidth: true
            spacing: 16

            Button
            {
                id: button_AddMod
                text: "Add mod"
                Layout.preferredHeight: 40

                icon.source: "qrc:/qt/qml/main/icons/add_box.svg"
                icon.color: "white"
                icon.width: 30
                icon.height: 30

                display: AbstractButton.TextBesideIcon

                font.bold: true
                palette.buttonText: "white"

                ToolTip.text: "Add a new mod from file"
                ToolTip.visible: hovered
                ToolTip.delay: 500

                background: Rectangle {

                    radius: 6

                    color: {
                        if (button_AddMod.down) {
                            return "#234e19"
                        } else if (button_AddMod.hovered) {
                            return "#2d7140"
                        } else {
                            return "#33914e"
                        }
                    }
                }

                onClicked: console.log("Open add mod dialog")
            }

            Item { Layout.fillWidth: true } // Spring

            Button
            {
                id: button_RefreshModList

                Layout.preferredHeight: 40

                icon.source: "qrc:/qt/qml/main/icons/sync.svg"
                icon.color: "white"
                icon.width: 30
                icon.height: 30

                display: AbstractButton.IconOnly

                font.bold: true
                palette.buttonText: "white"

                ToolTip.text: "Refresh mod list"
                ToolTip.visible: hovered
                ToolTip.delay: 500

                background: Rectangle {

                    radius: 6

                    color: {
                        if (button_CheckUpdates.down) {
                            return "#143f5c"
                        } else if (button_CheckUpdates.hovered) {
                            return "#1f5c87"
                        } else {
                            return "#237cb8"
                        }
                    }
                }

                onClicked: ModManager.reloadMods()
            }

            Button
            {
                id: button_CheckUpdates
                text: "Check updates"

                Layout.preferredHeight: 40

                icon.source: "qrc:/qt/qml/main/icons/check_update.svg"
                icon.color: "white"
                icon.width: 30
                icon.height: 30

                display: AbstractButton.TextBesideIcon

                font.bold: true
                palette.buttonText: "white"

                ToolTip.text: "Check for mod updates"
                ToolTip.visible: hovered
                ToolTip.delay: 500

                background: Rectangle {

                    radius: 6

                    color: {
                        if (button_CheckUpdates.down) {
                            return "#143f5c"
                        } else if (button_CheckUpdates.hovered) {
                            return "#1f5c87"
                        } else {
                            return "#237cb8"
                        }
                    }
                }

                onClicked: console.log("Checking for updates...")
            }

            Button
            {
                id: button_UpdateSelected
                text: "Update selected"

                Layout.preferredHeight: 40

                icon.source: "qrc:/qt/qml/main/icons/download.svg"
                icon.color: "white"
                icon.width: 30
                icon.height: 30

                display: AbstractButton.TextBesideIcon

                font.bold: true
                palette.buttonText: "white"

                ToolTip.text: "Update selected mods"
                ToolTip.visible: hovered
                ToolTip.delay: 500

                background: Rectangle {

                    radius: 6

                    color: {
                        if (button_UpdateSelected.down) {
                            return "#2a2a2a"
                        } else if (button_UpdateSelected.hovered) {
                            return "#535353"
                        } else {
                            return "#6c6c6c"
                        }
                    }
                }

                onClicked: console.log("Updating selected mods...")
            }

            Button
            {
                id: button_UpdateAll
                text: "Update all"

                Layout.preferredHeight: 40

                icon.source: "qrc:/qt/qml/main/icons/download_all.svg"
                icon.color: "#e0a23a"
                icon.width: 30
                icon.height: 30

                display: AbstractButton.TextBesideIcon

                font.bold: true
                palette.buttonText: "#e0a23a"

                ToolTip.text: "Update all mods"
                ToolTip.visible: hovered
                ToolTip.delay: 500

                background: Rectangle {

                    radius: 6

                    color: {
                        if (button_UpdateAll.down) {
                            return "#453713"
                        } else if (button_UpdateAll.hovered) {
                            return "#7c6220"
                        } else {
                            return "#a6842e"
                        }
                    }
                }

                onClicked: console.log("Updating all mods...")
            }


        }
        //endregion

        //region SEARCH BAR
        RowLayout
        {
            Layout.fillWidth: true
            spacing: 10

            TextField
            {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search installed mods..."
                font.pixelSize: 14
            }
        }
        //endregion

        //region MOD LIST
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#262626"
            border.color: "#3a3a3a"
            border.width: 1
            clip: true

            ListView {
                id: modListView
                anchors.fill: parent
                Layout.margins: 1
                model: ModListModel
                spacing: 0
                clip: true

                ScrollBar.vertical: ScrollBar {
                    id: vbar
                    policy: ScrollBar.AlwaysOn
                    width: 16
                    background: Item {}
                    // The Handle
                    contentItem: Rectangle {
                        implicitWidth: 16
                        radius: 8
                        color: vbar.pressed ? "#666666" : (vbar.hovered ? "#555555" : "#444444")
                    }
                }

                delegate: Rectangle {
                    id: modEntry
                    width: modListView.width
                    height: 64
                    color: "transparent"

                    // Divider
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: "#333333"
                        visible: index !== ModListModel.count - 1
                    }

                    RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 16

                        CheckBox {
                            id: selectForUpdate
                            checked: false
                            Layout.topMargin: 16
                            Layout.bottomMargin: 16
                            Layout.leftMargin: 0
                            Layout.rightMargin: 0
                            width: 15
                            height: 15
                            onCheckedChanged: {
                                if (checked) {
                                    console.log(name + " is selected for update")
                                } else {
                                    console.log(name + " is not selected for update anymore")
                                }
                            }
                        }

                        // Mod Icon
                        Rectangle {
                            id: icon
                            Layout.topMargin: 16
                            Layout.bottomMargin: 16
                            Layout.leftMargin: 0
                            Layout.rightMargin: 0
                            width: 36
                            height: 36
                            radius: 8
                            color: "#1D9E75"

                            IconImage {
                                source: "qrc:/qt/qml/main/icons/extension.svg"
                                color: "white"
                                anchors.centerIn: parent
                                sourceSize.width: 24
                                sourceSize.height: 24
                            }
                        }

                        ColumnLayout {
                            spacing: 4
                            clip: true

                            // Mod name, version layout
                            RowLayout {
                                spacing: 8

                                Label {
                                    text: name
                                    font.pixelSize: 14
                                    font.weight: Font.Medium
                                    color: "#e0e0e0"
                                    Layout.maximumWidth: 200
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: "v" + version
                                    font.pixelSize: 11
                                    color: "#888888"
                                }

                                Rectangle {
                                    visible: updateVersion !== "latest"
                                    radius: 6
                                    color: "#3a2f12"
                                    implicitWidth: updateLabel.width + 16
                                    implicitHeight: 18

                                    Label {
                                        id: updateLabel
                                        anchors.centerIn: parent
                                        text: "v" + updateVersion + " available"
                                        font.pixelSize: 11
                                        color: "#e0a23a"
                                    }
                                }
                                Rectangle {
                                    visible: updateVersion === "latest"
                                    radius: 6
                                    color: "#05552f"
                                    implicitWidth: latestLabel.width + 16
                                    implicitHeight: 18

                                    Label {
                                        id: latestLabel
                                        anchors.centerIn: parent
                                        text: "Latest version"
                                        font.pixelSize: 11
                                        color: "#00ff00"
                                    }
                                }
                            }

                            // Mod author, category layout
                            RowLayout {
                                Layout.maximumWidth: modEntry.width * 0.5
                                spacing: 8
                                clip: true

                                Label {
                                    text: "by " + author
                                    font.pixelSize: 12
                                    color: "#999999"
                                }

                                Label {
                                    text: " · "
                                    font.pixelSize: 12
                                    color: "#999999"
                                }

                                Repeater {
                                    model: tags
                                    delegate: Label {
                                        text: modelData
                                        font.pixelSize: 10
                                        color: tagColor(modelData)
                                        background: Rectangle {
                                            color: "#333333"
                                            radius: 4
                                        }
                                        padding: 2
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }
                        }

                        Item { Layout.fillWidth: true }



                        Button {
                            id: updateModButton
                            text: "Download update"
                            visible: updateVersion !== "latest"
                            highlighted: updateVersion !== "latest"
                            onClicked: console.log("Open add mod dialog")
                        }

                        ToolButton {
                            id: menuButton
                            text: "\u22EE"
                            onClicked: console.log("Mod options for " + name)
                        }

                    }
                }
            }
        }
        //endregion
    }
    //endregion
}