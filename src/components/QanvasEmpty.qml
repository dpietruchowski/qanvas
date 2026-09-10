import QtQuick.Layouts

import Themed.Components

ThemedEmpty {
    id: root

    property string iconName: ""

    Layout.fillWidth: true
    iconSource: Theme.iconSource(root.iconName)
}
