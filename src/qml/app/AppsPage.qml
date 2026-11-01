import QtQuick

import Qanvas
import Themed.Components

ThemedPage {
    id: root

    required property var session

    property string openedId: ""
    property bool locked: false
    property string removingId: ""

    signal appActivated(string id)
    signal backClicked()

    function reload() {
        stage.reload()
    }

    objectName: "appsPage"

    AppList {
        objectName: "appList"
        anchors.fill: parent
        visible: root.openedId === ""
        apps: AppRegistry.apps

        onAppActivated: function (id) { root.appActivated(id) }
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
        anchors.topMargin: Theme.agentBar.headerHeight + Theme.spacing.small
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
        messages: root.session.messages
        activity: root.session.activity
        busy: root.session.busy
        locked: root.locked

        onSent: function (text) { root.session.send(text) }
        onStopRequested: root.session.cancel()
        onRetryRequested: root.session.retry()
        onBackClicked: root.backClicked()
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
