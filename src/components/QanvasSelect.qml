import Themed.Components

ThemedComboBox {
    id: root

    property var options: []
    readonly property string value: root.currentText

    signal selected(string value)

    model: root.options

    onActivated: root.selected(root.currentText)
}
