import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedText {
    id: root

    property string type: "body"

    Layout.fillWidth: true
    textStyle: Theme.textStyle(root.type)
    horizontalAlignment: Theme.textAlignment(root.type)
}
