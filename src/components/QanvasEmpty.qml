import Themed.Components

ThemedEmpty {
    id: root

    property string iconName: ""

    iconSource: Theme.iconSource(root.iconName)
}
