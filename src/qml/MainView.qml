import QtQuick

import App.Components
import Chat.Components
import Qanvas
import Themed.Components

AppShell {
    id: root

    readonly property string openedAppId: chatSession.context.data?.id ?? ""
    readonly property bool chatLocked: ChatHost.busy && !chatSession.busy

    signal appReloadRequested()

    function appTitle(id) {
        const apps = AppRegistry.apps
        for (let i = 0; i < apps.length; ++i) {
            if (apps[i].id === id)
                return apps[i].title
        }
        return ""
    }

    function openApp(id) {
        chatSession.context = id === "" ? {} : {
            key: "app",
            title: root.appTitle(id),
            data: { id: id }
        }
    }

    function openConversation(appId, conversationId) {
        root.openApp(appId)
        chatSession.conversationId = conversationId
    }

    objectName: "mainView"

    property Component chatPage: ChatPage {
        session: chatSession
        locked: root.chatLocked

        onSettingsClicked: settings.item.open()
        onNewChatRequested: root.openConversation("", "")
        onConversationPicked: function (conversationId) {
            root.openConversation(ChatHost.appFor(conversationId), conversationId)
        }
    }

    property Component appPage: AppsPage {
        id: appsView

        session: chatSession
        openedId: root.openedAppId
        locked: root.chatLocked

        onAppActivated: function (id) {
            root.openConversation(id, ChatHost.conversationFor(id))
        }
        onBackClicked: root.openApp("")

        Connections {
            target: root

            function onAppReloadRequested() {
                appsView.reload()
            }
        }
    }

    pages: [
        { name: "navChat", icon: Theme.icons.chat, screen: root.chatPage },
        { name: "navApp", icon: Theme.icons.aiApp, screen: root.appPage }
    ]

    onOpenedAppIdChanged: ChatHost.lastAppId = root.openedAppId

    Component.onCompleted: {
        if (ChatHost.lastAppId !== "")
            root.openConversation(ChatHost.lastAppId, ChatHost.conversationFor(ChatHost.lastAppId))
    }

    ChatSession {
        id: chatSession

        objectName: "chatSession"
        chat: Chat

        onConversationIdChanged: {
            if (chatSession.conversationId !== "" && root.openedAppId !== ""
                    && ChatHost.appFor(chatSession.conversationId) !== root.openedAppId)
                ChatHost.bind(root.openedAppId, chatSession.conversationId)
        }
    }

    Connections {
        target: ChatHost

        function onAppShown(id) {
            if (chatSession.conversationId !== "")
                ChatHost.bind(id, chatSession.conversationId)
            const sameApp = root.openedAppId === id
            root.openApp(id)
            if (sameApp)
                root.appReloadRequested()
            if (root.currentName !== "navApp")
                root.selectNext()
        }
    }

    Connections {
        target: AppRegistry

        function onAppRemoving(id) {
            if (root.openedAppId === id)
                root.openApp("")
        }
    }

    Loader {
        id: settings

        sourceComponent: Qt.platform.os === "android" ? providerSettings : chatSettings
    }

    Component {
        id: chatSettings

        ChatSettingsPanel {
            model: Chat.model
            reasoningEffort: Chat.reasoningEffort

            onModelChosen: function (model) { Chat.model = model }
            onReasoningEffortChosen: function (effort) { Chat.reasoningEffort = effort }
        }
    }

    Component {
        id: providerSettings

        SettingsPanel {
            providers: ChatHost.providerSettings.providers
            provider: ChatHost.providerSettings.provider
            modelName: Chat.model
            reasoning: Chat.reasoningEffort
            apiKeyPreview: ChatHost.providerSettings.apiKeyPreview
            apiKeyHint: ChatHost.providerSettings.apiKeyHint

            onProviderSelected: function (provider) { ChatHost.providerSettings.provider = provider }
            onModelNameEdited: function (modelName) { Chat.model = modelName }
            onReasoningSelected: function (reasoning) { Chat.reasoningEffort = reasoning }
            onApiKeyEdited: function (apiKey) { ChatHost.providerSettings.apiKey = apiKey }
        }
    }
}
