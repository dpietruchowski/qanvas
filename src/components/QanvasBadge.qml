import Themed.Components

ThemedBadge {
    id: root

    property string type: ""

    badgeStyle: Theme.badgeStyle(root.type)
}
