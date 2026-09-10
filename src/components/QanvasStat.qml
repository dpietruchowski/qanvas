import QtQuick.Layouts

import Themed.Components

ThemedStat {
    id: root

    property string type: ""

    Layout.fillWidth: true
    valueStyle: Theme.statValueStyle(root.type)
}
