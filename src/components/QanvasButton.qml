import Themed.Components

ThemedButton {
    id: root

    property string type: "primary"
    property string iconName: ""

    buttonStyle: Theme.buttonStyle(root.type)
    buttonSize: Theme.buttonSize(root.type)
    iconSource: Theme.iconSource(root.iconName)
    circular: Theme.hasType(root.type, "circle")
    pill: Theme.hasType(root.type, "rounded")
}
