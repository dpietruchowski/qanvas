import QtQuick

import App.Components
import Themed.Components

AppShell {
    id: root

    property string provider: "OpenRouter"
    property string modelName: "google/gemma-4-31b-it:free"
    property string reasoning: "Medium"
    property string apiKey: "sk-or-v1-2f6c9b1de4"

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

        onSettingsClicked: settingsPanel.open()
    }

    pages: [
        { name: "navChat", icon: Theme.icons.chat, screen: root.chatPage }
    ]

    SettingsPanel {
        id: settingsPanel

        provider: root.provider
        modelName: root.modelName
        reasoning: root.reasoning
        apiKey: root.apiKey

        onProviderSelected: function (provider) { root.provider = provider }
        onModelNameEdited: function (modelName) { root.modelName = modelName }
        onReasoningSelected: function (reasoning) { root.reasoning = reasoning }
    }
}
