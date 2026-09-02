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

    property Component appPage: AppPage {
        id: appView

        appId: agent.lastAppId

        Connections {
            target: agent

            function onAppCreated(id) {
                appView.reload()
            }
        }
    }

    pages: [
        { name: "navChat", icon: Theme.icons.chat, screen: root.chatPage },
        { name: "navApp", icon: Theme.icons.app, screen: root.appPage }
    ]

    ChatAgent {
        id: agent

        objectName: "chatAgent"

        onAppCreated: {
            if (root.currentName !== "navApp")
                root.selectNext()
        }
    }

    SettingsPanel {
        id: settingsPanel

        providers: agent.providers
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
