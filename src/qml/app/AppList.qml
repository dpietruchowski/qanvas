import QtQuick

import Themed.Components

ListView {
    id: root

    property var apps: []

    signal appActivated(string id)

    objectName: "appList"
    model: root.apps
    spacing: Theme.appList.spacing
    clip: true

    delegate: AppCard {
        required property var modelData

        width: root.width
        title: modelData.title
        subtitle: modelData.id

        onClicked: root.appActivated(modelData.id)
    }
}
