import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedCard {
    id: root

    property string title: ""
    property string subtitle: ""

    signal removeClicked()

    objectName: "appCard"
    clickable: true
    margins: Theme.appList.cardPadding
    implicitHeight: Math.max(row.implicitHeight + root.margins * 2, Theme.appList.cardMinHeight)

    RowLayout {
        id: row

        spacing: Theme.appList.actionSpacing

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.appList.lineSpacing

            Text {
                Layout.fillWidth: true
                text: root.title
                elide: Text.ElideRight
                color: Theme.appList.titleColor
                font.pixelSize: Theme.appList.titleFontSize
            }

            Text {
                Layout.fillWidth: true
                text: root.subtitle
                elide: Text.ElideRight
                color: Theme.appList.subtitleColor
                font.pixelSize: Theme.appList.subtitleFontSize
            }
        }

        ThemedButton {
            objectName: "appRemoveButton"
            Layout.alignment: Qt.AlignVCenter
            iconSource: Theme.icons.remove
            buttonSize: Theme.button.icon
            buttonStyle: Theme.button.ghost
            onClicked: root.removeClicked()
        }
    }
}
