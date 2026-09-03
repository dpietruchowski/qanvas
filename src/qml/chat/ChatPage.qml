import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedPage {
    id: root

    property alias messages: messageList.messages
    property alias activities: messageList.activities
    property alias busy: messageList.busy
    property string userRole: "user"

    signal sent(string text)
    signal settingsClicked()
    signal newChatRequested()

    objectName: "chatPage"
    showBackButton: true

    headerAction: ThemedRow {
        spacing: Theme.spacing.small

        ThemedButton {
            objectName: "chatNewButton"
            Layout.alignment: Qt.AlignVCenter
            buttonSize: Theme.button.small
            buttonStyle: Theme.button.subtle
            pill: true
            text: qsTr("New")
            iconSource: Theme.icons.plus
            iconColor: Theme.colors.primary
            enabled: !root.busy

            onClicked: root.newChatRequested()
        }

        ThemedButton {
            objectName: "chatSettingsButton"
            Layout.alignment: Qt.AlignVCenter
            buttonSize: Theme.button.icon
            buttonStyle: Theme.button.ghost
            iconSource: Theme.icons.settings

            onClicked: root.settingsClicked()
        }
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
