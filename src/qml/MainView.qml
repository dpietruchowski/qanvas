import QtQuick

import App.Components
import Themed.Components

AppShell {
    id: root

    property Component chatPage: ChatPage {
        messages: [
            { sender: "user", text: qsTr("something to track which plants I watered") },
            { sender: "agent", text: qsTr("Which plants do you have?") },
            { sender: "user", text: qsTr("monstera, ficus, two basils on the sill") },
            { sender: "agent", text: qsTr("Building it now.") }
        ]
        busy: true

        onSent: function (text) {
            messages = messages.concat([{ sender: "user", text: text }])
        }
    }

    pages: [
        { name: "navChat", icon: Theme.icons.chat, screen: root.chatPage }
    ]
}
