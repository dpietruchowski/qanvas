import QtQuick
import QtQuick.Window

Window {
    id: root

    visible: true
    width: 480
    height: 800
    title: qsTr("Qanvas")
    color: "#101418"

    Text {
        anchors.centerIn: parent
        text: qsTr("Hello World")
        color: "#e6edf3"
        font.pixelSize: 32
    }
}
