import QtQuick.Layouts

import Themed.Components

ThemedSegmented {
    id: root

    property string value: options.length > 0 ? String(options[0]) : ""

    signal selected(string value)

    Layout.fillWidth: true
    currentIndex: root.options.indexOf(root.value)

    onActivated: function(index) {
        root.value = String(root.options[index])
        root.selected(root.value)
    }
}
