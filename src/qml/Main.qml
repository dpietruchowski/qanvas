import QtQuick
import QtQuick.Controls

import Themed.Components
import App.Components

ApplicationWindow {
    id: appWindow

    objectName: "mainWindow"
    width: 400
    height: 800
    visible: true
    title: qsTr("Qanvas")
    color: Theme.colors.background

    AppView {
        anchors.fill: parent
        systemBarColor: Theme.colors.background
        backgroundColor: Theme.colors.background

        Loader {
            id: mainLoader

            objectName: "mainLoader"
            anchors.fill: parent
            asynchronous: true
            source: "MainView.qml"
        }
    }

    Component.onCompleted: {
        Theme.applicationWidth = appWindow.width
        Theme.applicationHeight = appWindow.height
    }

    onWidthChanged: Theme.applicationWidth = width
    onHeightChanged: Theme.applicationHeight = height
}
