import QtQuick
import QtQuick.Controls

import Themed.Components

Item {
    id: root

    property string placeholder: qsTr("Reply")
    readonly property alias text: field.text

    signal sent(string text)

    function send() {
        if (field.text.trim() === "")
            return
        root.sent(field.text.trim())
        field.text = ""
    }

    implicitHeight: Theme.input.height

    Rectangle {
        id: pill

        anchors.fill: parent
        radius: Theme.input.radius
        color: Theme.input.background
        border.width: Theme.border.thin
        border.color: field.activeFocus ? Theme.input.borderFocused : Theme.input.border

        Behavior on border.color { ColorAnimation { duration: 150 } }

        TextField {
            id: field

            objectName: root.objectName !== "" ? root.objectName + "Input" : ""
            anchors.left: parent.left
            anchors.leftMargin: Theme.input.paddingH
            anchors.right: sendButton.left
            anchors.rightMargin: Theme.spacing.small
            anchors.verticalCenter: parent.verticalCenter
            placeholderText: root.placeholder
            placeholderTextColor: Theme.colors.textPlaceholder
            color: Theme.colors.textPrimary
            font.pixelSize: Theme.input.fontSize
            verticalAlignment: Text.AlignVCenter
            inputMethodHints: Qt.ImhNoPredictiveText
            background: null

            onAccepted: root.send()
        }

        ThemedButton {
            id: sendButton

            objectName: root.objectName !== "" ? root.objectName + "Send" : ""
            anchors.right: parent.right
            anchors.rightMargin: (parent.height - height) / 2
            anchors.verticalCenter: parent.verticalCenter
            buttonSize: Theme.button.circle
            buttonStyle: Theme.button.primary
            circular: true
            iconSource: Theme.icons.send

            onClicked: root.send()
        }
    }
}
