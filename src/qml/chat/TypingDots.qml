import QtQuick

import Themed.Components

Row {
    id: root

    property bool running: true
    property color color: Theme.colors.primary
    property color trailingColor: Theme.colors.textDisabled

    spacing: Theme.chat.dotSpacing

    Repeater {
        model: 3

        Rectangle {
            width: Theme.chat.dotSize
            height: Theme.chat.dotSize
            radius: width / 2
            color: index === 0 ? root.color : root.trailingColor
            opacity: 0.35

            SequentialAnimation on opacity {
                running: root.running
                loops: Animation.Infinite

                PauseAnimation { duration: index * 160 }
                NumberAnimation { to: 1.0; duration: 320; easing.type: Easing.InOutQuad }
                NumberAnimation { to: 0.35; duration: 320; easing.type: Easing.InOutQuad }
                PauseAnimation { duration: (2 - index) * 160 }
            }
        }
    }
}
