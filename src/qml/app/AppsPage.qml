import QtQuick

import Qanvas
import Themed.Components

ThemedPage {
    id: root

    property string openedId: ""
    property string removingId: ""
    property alias activities: agentBar.activities
    property alias busy: agentBar.busy

    signal appActivated(string id)
    signal sent(string text)

    function open(id) {
        if (root.openedId === id)
            stage.reload()
        else
            root.openedId = id
    }

    objectName: "appsPage"
    showBackButton: root.openedId !== ""

    onBackClicked: root.openedId = ""

    Connections {
        target: AppRegistry

        function onAppRemoving(id) {
            if (root.openedId === id)
                root.openedId = ""
        }
    }

    AppList {
        objectName: "appList"
        anchors.fill: parent
        visible: root.openedId === ""
        apps: AppRegistry.apps

        onAppActivated: function (id) {
            root.open(id)
            root.appActivated(id)
        }
        onAppRemoveRequested: function (id, title) {
            root.removingId = id
            removeDialog.message = qsTr("Delete \"%1\" with everything it stored?").arg(title)
            removeDialog.open()
        }
    }

    Text {
        objectName: "appsPlaceholder"
        anchors.centerIn: parent
        visible: root.openedId === "" && AppRegistry.apps.length === 0
        text: qsTr("No apps yet")
        color: Theme.appList.emptyColor
        font.pixelSize: Theme.appList.emptyFontSize
    }

    AppStage {
        id: stage

        anchors.fill: parent
        visible: root.openedId !== ""
        appId: root.openedId
    }

    AppAgentBar {
        id: agentBar

        objectName: "appAgentBar"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        z: 1
        visible: root.openedId !== ""

        onSent: function (text) { root.sent(text) }
    }

    ThemedDialog {
        id: removeDialog

        objectName: "appRemoveDialog"
        dialogTitle: qsTr("Delete app")
        dialogType: "danger"
        acceptText: qsTr("Delete")
        showRejectButton: true

        onAccepted: AppRegistry.remove(root.removingId)
    }
}
