import Themed.Components

ThemedSpinBox {
    id: root

    signal changed(int value)

    onValueModified: root.changed(root.value)
}
