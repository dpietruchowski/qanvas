import Themed.Components

ThemedListItem {
    id: root

    property string iconName: ""

    iconSource: Theme.iconSource(root.iconName)
    clickable: true
}
