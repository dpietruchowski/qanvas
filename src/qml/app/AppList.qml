import QtQuick

import Themed.Components

ThemedGridView {
    id: root

    property var apps: []

    signal appActivated(string id)
    signal appRemoveRequested(string id, string title)

    objectName: "appList"
    model: root.apps
    columns: Math.max(1, Math.floor(root.width / Theme.appList.tileMinWidth))
    cellHeight: Theme.appList.tileHeight

    delegate: Item {
        id: cell

        required property var modelData

        width: root.cellWidth
        height: root.cellHeight

        AppCard {
            anchors.fill: parent
            anchors.margins: Theme.appList.tileGap / 2
            title: cell.modelData.title
            icon: cell.modelData.icon

            onClicked: root.appActivated(cell.modelData.id)
            onRemoveClicked: root.appRemoveRequested(cell.modelData.id, cell.modelData.title)
        }
    }
}
