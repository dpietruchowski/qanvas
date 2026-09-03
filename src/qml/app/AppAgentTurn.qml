import QtQuick

import Qanvas
import Themed.Components

Column {
    id: root

    property var askedMessage: null
    property var replyMessages: []
    property var activities: []
    property bool busy: false

    readonly property bool working: root.activities.length > 0
                                    && root.activities[root.activities.length - 1].running

    spacing: Theme.chat.messageSpacing

    ChatBubble {
        objectName: "appAgentAsk"
        width: root.width
        visible: root.askedMessage !== null
        text: root.askedMessage ? root.askedMessage.text : ""
        style: Theme.chatBubble.user
        maxWidth: root.width * Theme.chat.bubbleMaxWidthRatio
    }

    ChatActivityLog {
        objectName: "appAgentLog"
        width: root.width
        activities: root.activities
    }

    Repeater {
        model: root.replyMessages

        ChatBubble {
            objectName: "appAgentReply" + index
            width: root.width
            text: modelData.text
            style: Theme.chatBubble.agent
            maxWidth: root.width * Theme.chat.bubbleMaxWidthRatio
        }
    }

    ChatThinking {
        objectName: "appAgentThinking"
        width: root.width
        visible: root.busy && !root.working
        running: visible
    }
}
