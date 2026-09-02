import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedPage {
    id: root

    property alias messages: messageList.messages
    property alias busy: messageList.busy
    property string userRole: "user"

    signal sent(string text)
    signal settingsClicked()

    objectName: "chatPage"
    showBackButton: true

    headerAction: ThemedButton {
        objectName: "chatSettingsButton"
        buttonSize: Theme.button.icon
        buttonStyle: Theme.button.ghost
        iconSource: Theme.icons.settings

        onClicked: root.settingsClicked()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacing.medium

        ChatMessageList {
            id: messageList

            objectName: "chatMessageList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            userRole: root.userRole
        }

        ChatComposer {
            objectName: "chatComposer"
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.spacing.small

            onSent: function (text) { root.sent(text) }
        }
    }
}
