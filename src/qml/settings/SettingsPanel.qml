import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedPanel {
    id: root

    property var providers: ["OpenAI", "OpenRouter"]
    property string provider: ""
    property string modelName: ""
    property var reasoningLevels: ["Minimal", "Low", "Medium", "High"]
    property string reasoning: ""
    property string apiKey: ""

    signal providerSelected(string provider)
    signal modelNameEdited(string modelName)
    signal reasoningSelected(string reasoning)
    signal apiKeyReplaceRequested()

    objectName: "settingsPanel"
    contentSpacing: Theme.settings.sectionSpacing

    SettingsSection {
        Layout.fillWidth: true
        label: qsTr("Provider")

        SettingsOptions {
            Layout.fillWidth: true
            options: root.providers
            value: root.provider

            onSelected: function (option) { root.providerSelected(option) }
        }
    }

    SettingsSection {
        Layout.fillWidth: true
        label: qsTr("Model")

        SettingsField {
            objectName: "settingsModel"
            Layout.fillWidth: true
            text: root.modelName
            monospace: true

            onEdited: function (text) { root.modelNameEdited(text) }
        }
    }

    SettingsSection {
        Layout.fillWidth: true
        label: qsTr("Reasoning")

        SettingsOptions {
            Layout.fillWidth: true
            options: root.reasoningLevels
            value: root.reasoning

            onSelected: function (option) { root.reasoningSelected(option) }
        }
    }

    SettingsSection {
        Layout.fillWidth: true
        label: qsTr("API key")

        SettingsField {
            objectName: "settingsApiKey"
            Layout.fillWidth: true
            text: root.apiKey
            masked: true
            actionText: qsTr("Replace")

            onActionTriggered: root.apiKeyReplaceRequested()
        }
    }
}
