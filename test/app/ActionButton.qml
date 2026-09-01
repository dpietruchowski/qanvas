import QtQuick

// A sibling file, imported by nothing: .qml files in one directory see each
// other implicitly, so a generated app needs no qmldir and no import to split
// itself into components.
Rectangle {
    id: control

    property alias text: label.text
    // The one call to action per row; everything else is a quiet outline.
    property bool accent: false
    property bool enabled: true
    signal clicked()

    Theme { id: theme }

    implicitWidth: label.implicitWidth + 40
    implicitHeight: 42
    radius: theme.radius
    opacity: control.enabled ? 1 : 0.4
    color: control.accent
        ? (area.pressed ? Qt.darker(theme.accent, 1.25) : theme.accent)
        : (area.pressed ? theme.surfaceRaised : "transparent")
    border.color: control.accent ? "transparent" : theme.line
    border.width: control.accent ? 0 : 1

    Behavior on color {
        ColorAnimation { duration: 90 }
    }

    Text {
        id: label

        objectName: control.objectName + "Label"
        anchors.centerIn: parent
        color: control.accent ? theme.accentText : theme.text
        font.pixelSize: 14
        font.bold: control.accent
    }

    MouseArea {
        id: area

        objectName: control.objectName + "Area"
        anchors.fill: parent
        enabled: control.enabled
        onClicked: control.clicked()
    }
}
