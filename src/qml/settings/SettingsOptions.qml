import QtQuick
import QtQuick.Layouts

import Themed.Components

RowLayout {
    id: root

    property var options: []
    property string value: ""

    signal selected(string option)

    spacing: Theme.settings.optionSpacing

    Repeater {
        model: root.options

        ThemedButton {
            required property string modelData

            objectName: "settingsOption" + modelData
            text: modelData
            Layout.fillWidth: true
            buttonSize: Theme.button.medium
            buttonStyle: modelData === root.value ? Theme.button.primary : Theme.button.subtle
            pill: true

            onClicked: root.selected(modelData)
        }
    }
}
