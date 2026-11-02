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

    Item {
        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.appList.tilePadding
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
            anchors.margins: Theme.appList.removeInset
            iconSource: Theme.icons.remove
            buttonSize: Theme.button.iconSmall
            buttonStyle: Theme.button.ghost
            iconColor: Theme.appList.removeColor
            onClicked: root.removeClicked()
        }
    }
}
