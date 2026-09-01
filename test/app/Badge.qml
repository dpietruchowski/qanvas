import QtQuick

// A pill. Used for the app id and the note count, so it takes its own colour.
Rectangle {
    id: badge

    property alias text: label.text
    property color textColor: theme.accent

    Theme { id: theme }

    implicitWidth: label.implicitWidth + 18
    implicitHeight: label.implicitHeight + 10
    radius: height / 2
    color: Qt.rgba(badge.textColor.r, badge.textColor.g, badge.textColor.b, 0.14)
    border.color: Qt.rgba(badge.textColor.r, badge.textColor.g, badge.textColor.b, 0.35)
    border.width: 1

    Text {
        id: label

        objectName: badge.objectName + "Label"
        anchors.centerIn: parent
        color: badge.textColor
        font.pixelSize: 12
        font.bold: true
    }
}
