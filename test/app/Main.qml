import QtQuick

// The host's own QML module. An app's sibling files need no import at all, so
// this line is the whole question: can a loaded app reach a type the runtime
// registered, or only its own directory?
import Qanvas

// A checked-in stand-in for a generated app: the same shape as anything the
// agent writes into apps/<id>/, but stable enough to assert against.
//
// Every item the UI automation server has to reach carries an objectName —
// these are the app's own names, resolved inside the app's subtree, which is
// how a test tells "the app loaded" apart from "the host drew a placeholder".
Rectangle {
    id: root

    objectName: "testApp"

    readonly property var rows: (App.revision,
        App.query("SELECT count(*) AS n FROM notes"))
    readonly property int noteCount: rows.length ? rows[0].n : -1

    Theme { id: theme }

    color: theme.background

    Component.onCompleted: App.exec(
        "CREATE TABLE IF NOT EXISTS notes (id INTEGER PRIMARY KEY, text TEXT)")

    Item {
        id: header

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: theme.pad
        }
        height: 44

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            Text {
                objectName: "titleText"
                text: "Notes"
                color: theme.text
                font.pixelSize: 22
                font.bold: true
            }

            Text {
                objectName: "appIdText"
                text: "app id: " + App.id
                color: theme.textDim
                font.pixelSize: 12
            }
        }

        Badge {
            objectName: "badge"
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            text: root.noteCount + (root.noteCount === 1 ? " note" : " notes")
        }
    }

    Panel {
        id: hostPanel

        objectName: "hostPanel"
        anchors {
            left: parent.left
            right: parent.right
            top: header.bottom
            margins: theme.pad
        }
        title: "FROM THE HOST MODULE"

        Text {
            objectName: "hostPanelText"
            text: "Panel comes from Qanvas, not from this directory"
            color: theme.textDim
            font.pixelSize: 12
        }
    }

    Card {
        id: notesCard

        objectName: "notesCard"
        anchors {
            left: parent.left
            right: parent.right
            top: hostPanel.bottom
            bottom: actions.top
            margins: theme.pad
            bottomMargin: theme.gap
        }
        title: "Stored in this app's database"
        fills: true

        NoteList {
            objectName: "noteList"
            width: parent.width
            // Card lays its content out in a Column, which does not stretch its
            // children, so the list takes the height the card has left.
            height: notesCard.height - y - 2 * theme.pad
        }
    }

    Row {
        id: actions

        objectName: "actionsRow"
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: theme.pad
        }
        spacing: theme.gap

        ActionButton {
            objectName: "addNoteButton"
            width: (actions.width - theme.gap) * 0.6
            text: "Add note"
            accent: true
            // Numbered from the id the row is about to get, not from the
            // count: after deleting from the middle the two disagree, and it
            // is the id the list shows.
            onClicked: App.exec("INSERT INTO notes (text) VALUES ('note ' || "
                                + "(SELECT ifnull(max(id), 0) + 1 FROM notes))")
        }

        ActionButton {
            objectName: "clearNotesButton"
            width: actions.width - theme.gap - (actions.width - theme.gap) * 0.6
            text: "Clear"
            enabled: root.noteCount > 0
            onClicked: App.exec("DELETE FROM notes")
        }
    }
}
