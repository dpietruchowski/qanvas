import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedPanel {
    id: root

    property var providers: ["OpenAI", "OpenRouter"]
    property string provider: ""
    property string modelName: ""
    property var agents: []
    property var reasoningLevels: ["Minimal", "Low", "Medium", "High"]
    property string reasoning: ""
    property string apiKeyPreview: ""
    property string apiKeyHint: ""

    signal providerSelected(string provider)
    signal modelNameEdited(string modelName)
    signal agentModelEdited(string key, string model)
    signal reasoningSelected(string reasoning)
    signal apiKeyEdited(string apiKey)

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

    Repeater {
        model: root.agents

        SettingsSection {
            id: agentSection

            required property var modelData

            Layout.fillWidth: true
            label: qsTr("%1 model").arg(agentSection.modelData.title)

            SettingsField {
                objectName: "settingsAgentModel_" + agentSection.modelData.key
                Layout.fillWidth: true
                text: agentSection.modelData.chosen
                placeholder: agentSection.modelData.model
                monospace: true

                onEdited: function (text) {
                    root.agentModelEdited(agentSection.modelData.key, text)
                }
            }
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

        SettingsSecretField {
            objectName: "settingsApiKey"
            Layout.fillWidth: true
            preview: root.apiKeyPreview
            placeholder: root.apiKeyHint

            onAccepted: function (apiKey) { root.apiKeyEdited(apiKey) }
        }
    }
}
