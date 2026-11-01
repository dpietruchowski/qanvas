import QtQuick

Item {
    id: root

    property string type: ""
    property bool running: false
    property int elapsed: 0
    property int centiseconds: 0
    readonly property bool precise: Theme.hasType(root.type, "centiseconds")
    readonly property string text: {
        const clock = Math.floor(elapsed / 60) + ":" + (elapsed % 60 < 10 ? "0" : "") + elapsed % 60
        if (!root.precise)
            return clock
        const hundredths = root.centiseconds % 100
        return clock + "." + (hundredths < 10 ? "0" : "") + hundredths
    }

    property double base: 0
    property double startedAt: 0

    function sync() {
        const seconds = root.base + (Date.now() - root.startedAt) / 1000
        root.centiseconds = Math.floor(seconds * 100)
        root.elapsed = Math.floor(seconds)
    }

    function reset() {
        base = 0
        startedAt = Date.now()
        elapsed = 0
        centiseconds = 0
    }

    visible: false
    implicitWidth: 0
    implicitHeight: 0

    onRunningChanged: {
        if (running) {
            startedAt = Date.now()
        } else {
            sync()
            base = centiseconds / 100
        }
    }

    Timer {
        interval: root.precise ? 30 : 250
        repeat: true
        running: root.running

        onTriggered: root.sync()
    }
}
