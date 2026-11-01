import QtQuick
import QtQuick.Layouts

import Chat.Components
import Themed.Components

ThemedPage {
    id: root

    required property var session

    property bool locked: false

    readonly property var chat: root.session.chat

    signal settingsClicked()
    signal newChatRequested()
    signal conversationPicked(string conversationId)

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

            onClicked: root.newChatRequested()
        }

        ThemedButton {
            objectName: "chatHistoryButton"
            Layout.alignment: Qt.AlignVCenter
            buttonSize: Theme.button.icon
            buttonStyle: Theme.button.ghost
            iconSource: Theme.icons.list
            Accessible.name: qsTr("Conversations")

            onClicked: historyDrawer.open()
        }

        ThemedButton {
            objectName: "chatSettingsButton"
            Layout.alignment: Qt.AlignVCenter
            buttonSize: Theme.button.icon
            buttonStyle: Theme.button.ghost
            iconSource: Theme.icons.sliders
            Accessible.name: qsTr("Chat settings")

            onClicked: root.settingsClicked()
        }
    }

    ThemedSideDrawer {
        id: historyDrawer

        objectName: "chatHistoryDrawer"
        drawerTitle: qsTr("Conversations")

        ThemedEmpty {
            objectName: "chatHistoryEmpty"
            Layout.fillWidth: true
            visible: historyList.count === 0
            iconSource: Theme.icons.chat
            text: qsTr("No conversations yet")
        }

        ChatConversationList {
            id: historyList

            objectName: "chatConversationList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: count > 0
            model: root.chat.conversations

            onOpened: function (conversationId) {
                historyDrawer.close()
                root.conversationPicked(conversationId)
            }
            onRemoveRequested: function (conversationId) { root.chat.remove(conversationId) }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        visible: root.chat.available
        spacing: Theme.spacing.medium

        ChatMessageList {
            objectName: "chatMessageList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.session.messages
            busy: root.session.busy
            activity: root.session.activity

            onRetryRequested: root.session.retry()
        }

        ChatComposer {
            objectName: "chatComposer"
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.spacing.small
            enabled: !root.locked
            busy: root.session.busy

            onSent: function (text) { root.session.send(text) }
            onStopRequested: root.session.cancel()
        }
    }

    ThemedEmpty {
        objectName: "chatUnavailable"
        anchors.fill: parent
        visible: !root.chat.available
        iconSource: Theme.icons.chat
        text: root.chat.unavailableReason
    }
}
