import QtQuick
import QtQuick.Layouts

import Themed.Components

ColumnLayout {
    id: root

    property string label: ""

    default property alias sectionContent: contentColumn.data

    spacing: Theme.settings.labelBottomSpacing

    Text {
        text: root.label
        color: Theme.settings.labelColor
        font.pixelSize: Theme.settings.labelSize
        font.letterSpacing: Theme.settings.labelSpacing
        font.capitalization: Font.AllUppercase
    }

    ColumnLayout {
        id: contentColumn

        Layout.fillWidth: true
        spacing: Theme.settings.optionSpacing
    }
}
