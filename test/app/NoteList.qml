import QtQuick

// Reads through the bridge on its own rather than being handed rows: each app
// component can query, and the revision dependency keeps every one of them in
// step with the last write.
Column {
    id: root

    property int limit: 5
    property var notes: (App.revision, App.query(
        "SELECT id, text FROM notes ORDER BY id DESC LIMIT ?", [limit]))

    spacing: 4

    Repeater {
        model: root.notes

        Text {
            objectName: "noteRow" + index
            text: modelData.id + ": " + modelData.text
            color: "#8b98a5"
            font.pixelSize: 12
        }
    }
}
