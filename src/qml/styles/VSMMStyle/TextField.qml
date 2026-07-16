import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl // PlaceholderText
import vsmm

T.TextField {
    id: control

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             placeholder.implicitHeight + topPadding + bottomPadding)

    padding: 6
    leftPadding: padding + 4

    font.pixelSize: Theme.fonts.label
    color: Theme.colors.text
    selectionColor: Theme.colors.textSelection
    selectedTextColor: Theme.colors.textSelected
    placeholderTextColor: Theme.colors.searchBarDefault
    verticalAlignment: TextInput.AlignVCenter

    PlaceholderText {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)

        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    // Default groove for a plain TextField; call sites that wrap the field in their
    // own colored Rectangle override `background` and keep their look.
    background: Rectangle {
        radius: Theme.radius.field
        color: Theme.colors.searchBarBg
        border.width: control.activeFocus ? 1 : 0
        border.color: Theme.colors.searchBarIcon
    }
}
