import QtQuick

import Qanvas
import Themed.Components

ThemedPage {
    id: root

    property string openedId: ""
    property string removingId: ""

    function open(id) {
        root.openedId = id
        stage.reload()
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

        onAppActivated: function (id) { root.open(id) }
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
