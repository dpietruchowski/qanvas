import QtQuick

import Qanvas
import Themed.Components

ThemedPage {
    id: root

    property string openedId: ""

    function open(id) {
        root.openedId = id
        stage.reload()
    }

    objectName: "appsPage"
    showBackButton: root.openedId !== ""

    onBackClicked: root.openedId = ""

    AppList {
        objectName: "appList"
        anchors.fill: parent
        visible: root.openedId === ""
        apps: AppRegistry.apps

        onAppActivated: function (id) { root.open(id) }
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
}
