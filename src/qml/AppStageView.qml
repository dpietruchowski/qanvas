import QtQuick
import QtQuick.Controls

import Qanvas

Rectangle {
    id: root

    objectName: "appStageView"
    color: "#101418"

    Rectangle {
        id: stage

        objectName: "stage"

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: footer.top
            margins: 12
        }
        color: "#0b0f14"
        border.color: "#243040"
        border.width: 1
        clip: true

        AppLoader {
            id: appLoader

            objectName: "appLoader"
            anchors.fill: parent
            appId: "app"
        }

        Text {
            objectName: "placeholderText"
            anchors.centerIn: parent
            anchors.margins: 16
            width: parent.width - 32
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            visible: appLoader.status !== AppLoader.Ready
            text: appLoader.status === AppLoader.Error ? appLoader.errorString
                                                       : qsTr("Nothing loaded")
            color: "#8b98a5"
            font.pixelSize: 14
        }
    }

    Column {
        id: footer

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 16
        }
        spacing: 8

        Text {
            objectName: "appDirText"
            width: root.width - 32
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideMiddle
            text: appLoader.appDir
            color: "#5c6b7a"
            font.pixelSize: 11
        }

        Button {
            objectName: "runAppButton"
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Run app")
            onClicked: appLoader.reload()
        }
    }
}
