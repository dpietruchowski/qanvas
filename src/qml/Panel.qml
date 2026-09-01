import QtQuick

// A host component. qt_add_qml_module registers every capitalised QML_FILES
// entry into the Qanvas module, so this is a type the runtime owns — nothing
// like the app's own files, which are found by sitting in the app's directory.
Rectangle {
    id: panel

    default property alias content: column.data
    property alias title: heading.text

    implicitHeight: column.implicitHeight + heading.height + 28
    color: "#132033"
    radius: 10
    border.color: "#2f4a6b"
    border.width: 1

    Text {
        id: heading

        objectName: panel.objectName + "Title"
        anchors {
            left: parent.left
            top: parent.top
            margins: 12
        }
        color: "#4c9aff"
        font.pixelSize: 12
        font.bold: true
    }

    Column {
        id: column

        anchors {
            left: parent.left
            right: parent.right
            top: heading.bottom
            margins: 12
            topMargin: 8
        }
        spacing: 6
    }
}
