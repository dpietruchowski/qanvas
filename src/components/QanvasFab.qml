import Themed.Components

ThemedButton {
    id: root

    property string iconName: "plus"

    iconSource: Theme.iconSource(root.iconName)
    buttonSize: Theme.button.circle
    buttonStyle: Theme.button.primary
    circular: true
}
