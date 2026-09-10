import QtQuick

Item {
    id: root

    property bool running: false
    property int elapsed: 0
    readonly property string text: Math.floor(elapsed / 60) + ":"
                                   + (elapsed % 60 < 10 ? "0" : "") + elapsed % 60

    property double base: 0
    property double startedAt: 0

    function reset() {
        base = 0
        startedAt = Date.now()
        elapsed = 0
    }

    visible: false
    implicitWidth: 0
    implicitHeight: 0

    onRunningChanged: {
        if (running)
            startedAt = Date.now()
        else
            base = elapsed
    }

    Timer {
        interval: 250
        repeat: true
        running: root.running

        onTriggered: root.elapsed = Math.floor(root.base + (Date.now() - root.startedAt) / 1000)
    }
}
