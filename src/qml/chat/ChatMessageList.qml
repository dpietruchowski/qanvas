import QtQuick

import Themed.Components

ListView {
    id: root

    property var messages: []
    property bool busy: false
    property string userRole: "user"

    model: root.messages
    spacing: Theme.chat.messageSpacing
    clip: true
    onCountChanged: Qt.callLater(positionViewAtEnd)

    delegate: ChatBubble {
        objectName: "chatMessage" + index
        width: root.width
        text: modelData.text
        style: modelData.sender === root.userRole ? Theme.chatBubble.user
                                                  : Theme.chatBubble.agent
        maxWidth: root.width * Theme.chat.bubbleMaxWidthRatio
    }

    footer: Item {
        width: root.width
        height: root.busy ? dots.height + Theme.chat.messageSpacing : 0
        visible: root.busy

        TypingDots {
            id: dots

            objectName: "typingDots"
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: Theme.chat.messageSpacing
            running: root.busy
        }
    }
}
