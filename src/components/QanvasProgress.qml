import QtQuick.Layouts

import Themed.Components

ThemedProgressBar {
    id: root

    property string type: ""

    Layout.fillWidth: true
    progressColor: Theme.progressColor(root.type)
}
