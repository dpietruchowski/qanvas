import QtQuick

// Reads through the bridge on its own rather than being handed rows: every app
// component can query, and the revision dependency keeps each of them in step
// with the last write, not just the one that wrote.
ListView {
    id: list

    Theme { id: theme }

    model: (App.revision, App.query("SELECT id, text FROM notes ORDER BY id DESC"))
    clip: true
    spacing: 6
    boundsBehavior: Flickable.StopAtBounds

    delegate: Rectangle {
        objectName: "noteRow" + index

        width: ListView.view.width
        height: 34
        radius: 6
        color: theme.surfaceRaised

        Text {
            objectName: "noteRow" + index + "Id"
            anchors {
                left: parent.left
                leftMargin: 10
                verticalCenter: parent.verticalCenter
            }
            width: 28
            text: modelData.id
            color: theme.textFaint
            font.pixelSize: 12
            font.family: "monospace"
        }

        Text {
            objectName: "noteRow" + index + "Text"
            anchors {
                left: parent.left
                leftMargin: 44
                right: removeButton.left
                rightMargin: 8
                verticalCenter: parent.verticalCenter
            }
            elide: Text.ElideRight
            text: modelData.text
            color: theme.text
            font.pixelSize: 13
        }

        // Deleting by id rather than by row: the model is a snapshot taken at
        // the last revision, so a row's position is only true until someone
        // else writes, but its id stays the row it was.
        Rectangle {
            id: removeButton

            objectName: "noteRow" + index + "Remove"
            anchors {
                right: parent.right
                rightMargin: 6
                verticalCenter: parent.verticalCenter
            }
            width: 24
            height: 24
            radius: 6
            color: removeArea.pressed ? theme.line : "transparent"

            Text {
                anchors.centerIn: parent
                text: "\u00d7"
                color: removeArea.pressed ? theme.text : theme.textFaint
                font.pixelSize: 16
            }

            MouseArea {
                id: removeArea

                objectName: "noteRow" + index + "RemoveArea"
                anchors.fill: parent
                onClicked: App.exec("DELETE FROM notes WHERE id = ?", [modelData.id])
            }
        }
    }

    Text {
        objectName: "emptyNotesText"
        anchors.centerIn: parent
        visible: list.count === 0
        text: "No notes yet"
        color: theme.textFaint
        font.pixelSize: 13
    }
}
