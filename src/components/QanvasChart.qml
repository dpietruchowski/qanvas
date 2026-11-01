import QtQuick.Layouts

import Themed.Components

ThemedBarChart {
    id: root

    Layout.fillWidth: true

    property string typeRole: ""
    property var typeFormat: null

    colorFormat: function (item) {
        return Theme.progressColor(String(root.formatted(root.typeFormat, root.typeRole, item)))
    }
}
