import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedCard {
    id: root

    property string title: ""
    property string icon: ""

    signal removeClicked()

    objectName: "appCard"
    clickable: true
    margins: Theme.appList.tilePadding

    Item {
        ColumnLayout {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: Theme.appList.tileSpacing

            ThemedIcon {
                objectName: "appIcon"
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Theme.appList.iconSize
                Layout.preferredHeight: Theme.appList.iconSize
                svgSource: Theme.iconSource(root.icon)
                color: Theme.appList.iconColor
            }

            Text {
                objectName: "appTitle"
                Layout.fillWidth: true
                text: root.title
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                maximumLineCount: Theme.appList.titleLines
                elide: Text.ElideRight
                color: Theme.appList.titleColor
                font.pixelSize: Theme.appList.titleFontSize
            }
        }

        ThemedButton {
            objectName: "appRemoveButton"
            anchors.top: parent.top
            anchors.right: parent.right
            iconSource: Theme.icons.remove
            buttonSize: Theme.button.icon
            buttonStyle: Theme.button.ghost
            onClicked: root.removeClicked()
        }
    }
}
