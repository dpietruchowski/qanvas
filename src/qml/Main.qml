import QtQuick
import QtQuick.Controls
import QtQuick.Window

import Themed.Components

Window {
    id: root

    objectName: "mainWindow"
    visible: true
    width: 400
    height: 800
    title: qsTr("Qanvas")
    color: Theme.colors.background

    onWidthChanged: Theme.applicationWidth = width
    onHeightChanged: Theme.applicationHeight = height

    ChatPage {
        id: chatPage

        anchors.fill: parent
        messages: [
            { sender: "user", text: qsTr("something to track which plants I watered") },
            { sender: "agent", text: qsTr("Which plants do you have?") },
            { sender: "user", text: qsTr("monstera, ficus, two basils on the sill") },
            { sender: "agent", text: qsTr("Building it now.") }
        ]
        busy: true

        onSent: function (text) {
            chatPage.messages = chatPage.messages.concat([{ sender: "user", text: text }])
        }
    }

    Component.onCompleted: {
        Theme.applicationWidth = root.width
        Theme.applicationHeight = root.height
    }
}
