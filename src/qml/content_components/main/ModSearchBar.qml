import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import vsmm
import VSMMStyle

TextField {
    id: searchInput

    Layout.fillWidth: true
    Layout.preferredHeight: 40

    iconSource: Theme.icons.searchIcon
    placeholderText: qsTr("Search installed mods...")

    onTextChanged: ModSortFilterModel.filterText = searchInput.text
}
