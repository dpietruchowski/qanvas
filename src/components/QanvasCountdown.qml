import QtQuick

Item {
    id: root

    property int seconds: 60
    property bool running: false
    property int remaining: seconds
    readonly property string text: Math.floor(remaining / 60) + ":"
                                   + (remaining % 60 < 10 ? "0" : "") + remaining % 60

    signal finished()

    function restart() {
        remaining = seconds
    }

    visible: false
    implicitWidth: 0
    implicitHeight: 0

    onRunningChanged: {
        if (running)
            remaining = seconds
    }

    Timer {
        interval: 1000
        repeat: true
        running: root.running && root.remaining > 0

        onTriggered: {
            root.remaining -= 1
            if (root.remaining === 0)
                root.finished()
        }
    }
}
