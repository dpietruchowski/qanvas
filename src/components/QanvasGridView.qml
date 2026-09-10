import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedGridView {
    id: root

    Layout.fillWidth: true

    property string textRole: ""

    signal itemClicked(int index, var item)

    cellHeight: cellWidth

    delegate: Item {
        id: cell

        required property var modelData
        required property int index

        width: root.cellWidth
        height: root.cellHeight

        QanvasCard {
            anchors.fill: parent
            anchors.margins: Theme.grid.spacing
            clickable: true

            onClicked: root.itemClicked(cell.index, cell.modelData)

            QanvasText {
                text: root.textRole ? String(cell.modelData[root.textRole])
                                    : String(cell.modelData)
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
