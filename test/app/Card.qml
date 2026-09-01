import QtQuick

// A titled surface. Three sections want the same frame, so pulling it out
// leaves each one with only what is actually different.
Rectangle {
    id: card

    default property alias content: column.data
    property alias title: heading.text
    // Set by the notes card, which fills the space it is given instead of
    // shrinking to its content.
    property bool fills: false

    Theme { id: theme }

    implicitHeight: fills ? 0 : column.implicitHeight + 2 * theme.pad
    color: theme.surface
    radius: theme.radius
    border.color: theme.line
    border.width: 1

    SectionLabel {
        id: heading

        anchors {
            left: parent.left
            top: parent.top
            margins: theme.pad
        }
        visible: text.length > 0
    }

    Column {
        id: column

        anchors {
            left: parent.left
            right: parent.right
            top: heading.visible ? heading.bottom : parent.top
            bottom: card.fills ? parent.bottom : undefined
            margins: theme.pad
            topMargin: heading.visible ? 10 : theme.pad
        }
        spacing: 8
    }
}
