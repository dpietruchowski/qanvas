import QtQuick
import QtQuick.Controls
import QtQuick.Window

import Qanvas

Window {
    id: root

    // objectName on everything the UI automation server has to reach: it walks
    // the object tree and dispatches by name, so an unnamed item is invisible
    // to it. See libs/automation/ui_driver.py.
    objectName: "mainWindow"
    visible: true
    width: 480
    height: 800
    title: qsTr("Qanvas")
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
            // A dev build resolves this under test/; a release, under app data.
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
            // Re-reads the files from disk. The stale-QML problem the old Loader
            // needed a "?v=N" cache-buster for is handled inside AppLoader.
            onClicked: appLoader.reload()
        }
    }
}
