import QtQuick.Layouts

import Themed.Components

ThemedListItem {
    id: root

    property string iconName: ""
    property string type: ""

    Layout.fillWidth: true
    iconSource: Theme.iconSource(root.iconName)
    done: Theme.hasType(root.type, "done")
    badgeStyle: Theme.badgeStyle(root.type)
    clickable: true
}
