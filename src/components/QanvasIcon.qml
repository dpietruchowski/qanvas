import Themed.Components

ThemedIcon {
    id: root

    property string iconName: ""
    property string type: "medium"

    svgSource: Theme.iconSource(root.iconName)
    color: Theme.colors.textSecondary
    width: Theme.hasType(root.type, "small") ? Theme.icon.small
         : Theme.hasType(root.type, "large") ? Theme.icon.large
         : Theme.icon.medium
    height: root.width
}
