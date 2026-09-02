import Themed.Components

ThemedText {
    id: root

    property string type: "body"

    textStyle: Theme.textStyle(root.type)
}
