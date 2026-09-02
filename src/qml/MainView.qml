import QtQuick

import App.Components
import Qanvas
import Themed.Components

AppShell {
    id: root

    property Component chatPage: ChatPage {
        messages: agent.messages
        busy: agent.busy

        onSent: function (text) { agent.send(text) }
        onSettingsClicked: settingsPanel.open()
    }

    pages: [
        { name: "navChat", icon: Theme.icons.chat, screen: root.chatPage }
    ]

    ChatAgent {
        id: agent

        objectName: "chatAgent"
    }

    SettingsPanel {
        id: settingsPanel

        provider: agent.provider
        modelName: agent.model
        reasoning: agent.reasoning
        apiKey: agent.apiKey
        apiKeyHint: agent.apiKeyHint

        onProviderSelected: function (provider) { agent.provider = provider }
        onModelNameEdited: function (modelName) { agent.model = modelName }
        onReasoningSelected: function (reasoning) { agent.reasoning = reasoning }
        onApiKeyEdited: function (apiKey) { agent.apiKey = apiKey }
        onApiKeyReplaceRequested: agent.apiKey = ""
    }
}
