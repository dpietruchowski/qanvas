import QtQuick

// A sibling file, imported by nothing: .qml files in the same directory see
// each other implicitly, so a generated app needs no qmldir and no import to
// split itself into components.
Rectangle {
    id: control

    property alias text: label.text
    signal clicked()

    implicitWidth: label.implicitWidth + 32
    implicitHeight: label.implicitHeight + 16
    radius: 6
    color: area.pressed ? "#2c3d52" : "#22303f"
    border.color: "#3a4d63"

    Text {
        id: label

        objectName: control.objectName + "Label"
        anchors.centerIn: parent
        color: "#e6edf3"
        font.pixelSize: 14
    }

    MouseArea {
        id: area

        objectName: control.objectName + "Area"
        anchors.fill: parent
        onClicked: control.clicked()
    }
}
