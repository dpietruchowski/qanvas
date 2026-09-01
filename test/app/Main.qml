import QtQuick

// A checked-in stand-in for a generated app: same shape as anything the agent
// writes into apps/<id>/, but stable enough to assert against. Load it with
// QANVAS_APPS_ROOT=<repo>/test and appId "app".
//
// Every item the UI automation server has to reach carries an objectName —
// these are the app's own names, resolved inside the app's subtree, which is
// how a test tells "the app loaded" apart from "the host drew a placeholder".
Rectangle {
    id: root

    objectName: "testApp"
    color: "#1b2430"

    property int noteCount: rows.length ? rows[0].n : -1
    // Re-runs whenever the app writes, so the view never goes stale.
    property var rows: (App.revision, App.query("SELECT count(*) AS n FROM notes"))

    Component.onCompleted: App.exec(
        "CREATE TABLE IF NOT EXISTS notes (id INTEGER PRIMARY KEY, text TEXT)")

    Column {
        objectName: "testAppColumn"
        anchors.centerIn: parent
        spacing: 12

        Text {
            objectName: "titleText"
            text: "test app"
            color: "#e6edf3"
            font.pixelSize: 20
        }

        Text {
            objectName: "appIdText"
            text: "id: " + App.id
            color: "#8b98a5"
            font.pixelSize: 14
        }

        Text {
            objectName: "noteCountText"
            text: "notes: " + root.noteCount
            color: "#8b98a5"
            font.pixelSize: 14
        }
    }

    MouseArea {
        objectName: "addNoteArea"
        anchors.fill: parent
        onClicked: App.exec("INSERT INTO notes (text) VALUES (?)", ["tap"])
    }
}
