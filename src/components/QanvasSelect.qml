import QtQuick.Layouts

import Themed.Components

ThemedComboBox {
    id: root

    Layout.fillWidth: true

    property var options: []
    property string value: ""

    signal selected(string value)

    model: root.options
    currentIndex: root.options.indexOf(root.value)

    onActivated: {
        root.value = root.currentText
        root.selected(root.currentText)
    }
}
