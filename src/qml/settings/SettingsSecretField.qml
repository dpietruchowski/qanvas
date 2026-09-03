import QtQuick

SettingsField {
    id: root

    property string preview: ""
    property bool editing: preview === ""
    property string pending: ""

    signal accepted(string value)

    text: root.editing ? "" : root.preview
    monospace: true
    masked: root.editing
    readOnly: !root.editing
    actionText: root.editing ? qsTr("Set") : qsTr("Edit")

    onPreviewChanged: {
        root.editing = root.preview === ""
        root.pending = ""
    }

    onEdited: function (text) { root.pending = text }

    onActionTriggered: {
        if (root.editing) {
            if (root.pending !== "")
                root.accepted(root.pending)
            root.pending = ""
            root.editing = root.preview === ""
        } else {
            root.pending = ""
            root.editing = true
            root.focusInput()
        }
    }
}
