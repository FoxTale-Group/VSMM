import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VSMM.Theme
import VSMM.ModSortFilterModel
import VSMMStyle

TextField {
    id: searchInput

    Layout.fillWidth: true
    Layout.preferredHeight: 40

    iconSource: Theme.icons.searchIcon
    placeholderText: qsTr("Search installed mods...")

    onTextChanged: ModSortFilterModel.filterText = searchInput.text
}
