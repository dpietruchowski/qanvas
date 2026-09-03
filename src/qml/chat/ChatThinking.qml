import QtQuick

import Themed.Components

ChatActivityLine {
    id: root

    property int seconds: 0

    text: "thinking"
    detail: root.seconds > 0 ? root.seconds + "s" : ""

    onRunningChanged: root.seconds = 0

    Timer {
        interval: Theme.activity.tickInterval
        repeat: true
        running: root.running
        onTriggered: root.seconds += 1
    }
}
