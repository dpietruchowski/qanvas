import QtQuick

import Themed.Components

ListView {
    id: root

    property var messages: []
    property var activities: []
    property bool busy: false
    property string userRole: "user"

    model: root.messages
    spacing: Theme.chat.messageSpacing
    clip: true
    onCountChanged: Qt.callLater(positionViewAtEnd)
    onActivitiesChanged: Qt.callLater(positionViewAtEnd)

    delegate: ChatBubble {
        objectName: "chatMessage" + index
        width: root.width
        text: modelData.text
        style: modelData.sender === root.userRole ? Theme.chatBubble.user
                                                  : Theme.chatBubble.agent
        maxWidth: root.width * Theme.chat.bubbleMaxWidthRatio
    }

    footer: Column {
        width: root.width
        height: visible ? implicitHeight : 0
        spacing: Theme.chat.messageSpacing
        topPadding: Theme.chat.messageSpacing
        visible: root.busy || root.activities.length > 0

        ChatActivityLog {
            objectName: "chatActivityLog"
            width: parent.width
            activities: root.activities
        }

        TypingDots {
            objectName: "typingDots"
            visible: root.busy
            running: root.busy
        }
    }
}
