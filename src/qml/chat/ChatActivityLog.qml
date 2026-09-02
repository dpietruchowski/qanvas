import QtQuick

import Themed.Components

Column {
    id: root

    property var activities: []

    spacing: Theme.activity.spacing
    visible: root.activities.length > 0

    Repeater {
        model: root.activities

        ChatActivityLine {
            objectName: "chatActivity" + index
            width: root.width
            text: modelData.text
            detail: modelData.detail
            running: modelData.running
            failed: modelData.failed
        }
    }
}
