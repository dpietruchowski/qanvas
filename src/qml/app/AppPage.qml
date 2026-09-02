import QtQuick

import Qanvas
import Themed.Components

ThemedPage {
    id: root

    property alias appId: appLoader.appId

    function reload() {
        appLoader.reload()
    }

    objectName: "appPage"

    Rectangle {
        anchors.fill: parent
        color: Theme.appStage.background
        radius: Theme.appStage.radius
        border.width: Theme.border.thin
        border.color: Theme.appStage.border
        clip: true

        AppLoader {
            id: appLoader

            objectName: "appLoader"
            anchors.fill: parent
        }

        Text {
            objectName: "appPlaceholder"
            anchors.centerIn: parent
            width: parent.width - Theme.appStage.placeholderPadding * 2
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            visible: appLoader.status !== AppLoader.Ready
            text: appLoader.status === AppLoader.Error ? appLoader.errorString
                                                       : qsTr("No app yet")
            color: Theme.appStage.placeholderText
            font.pixelSize: Theme.appStage.placeholderFontSize
        }
    }
}
