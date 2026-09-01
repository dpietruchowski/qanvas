import QtQuick

Rectangle {
    property alias text: label.text

    implicitWidth: label.implicitWidth + 12
    implicitHeight: label.implicitHeight + 6
    radius: height / 2
    color: "#3a4d63"

    Text {
        id: label
        objectName: "badgeLabel"
        anchors.centerIn: parent
        color: "#e6edf3"
        font.pixelSize: 11
    }
}
