import QtQuick
import QtQuick.Controls

import Themed.Components

Rectangle {
    id: root

    property string text: ""
    property string placeholder: ""
    property bool masked: false
    property bool monospace: false
    property bool readOnly: false
    property string actionText: ""

    signal edited(string text)
    signal actionTriggered()

    function focusInput() {
        field.forceActiveFocus()
    }

    onTextChanged: {
        if (field.text !== root.text)
            field.text = root.text
    }

    implicitHeight: Theme.settings.fieldHeight
    radius: Theme.settings.fieldRadius
    color: Theme.settings.fieldBackground
    border.width: Theme.border.thin
    border.color: field.activeFocus ? Theme.settings.fieldBorderFocused : Theme.settings.fieldBorder

    TextField {
        id: field

        objectName: root.objectName + "Input"
        anchors.fill: parent
        anchors.leftMargin: Theme.settings.fieldPadding
        anchors.rightMargin: actionButton.visible
                             ? actionButton.width + Theme.spacing.medium : Theme.settings.fieldPadding
        leftPadding: 0
        rightPadding: 0
        background: null
        text: root.text
        placeholderText: root.placeholder
        placeholderTextColor: Theme.colors.textPlaceholder
        color: Theme.colors.textPrimary
        font.pixelSize: Theme.settings.fieldFontSize
        font.family: root.monospace ? Theme.settings.monoFamily : Theme.settings.fontFamily
        echoMode: root.masked ? TextInput.Password : TextInput.Normal
        readOnly: root.readOnly
        activeFocusOnPress: !root.readOnly
        verticalAlignment: Text.AlignVCenter

        onTextEdited: root.edited(text)
    }

    ThemedButton {
        id: actionButton

        objectName: root.objectName + "Action"
        anchors.right: parent.right
        anchors.rightMargin: Theme.spacing.small
        anchors.verticalCenter: parent.verticalCenter
        visible: root.actionText !== ""
        text: root.actionText
        buttonSize: Theme.button.small
        buttonStyle: Theme.button.accent
        pill: true

        onClicked: root.actionTriggered()
    }
}
