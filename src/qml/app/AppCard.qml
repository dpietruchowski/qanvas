import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedCard {
    id: root

    property string title: ""
    property string subtitle: ""

    objectName: "appCard"
    clickable: true
    margins: Theme.appList.cardPadding
    implicitHeight: Math.max(lines.implicitHeight + root.margins * 2, Theme.card.sizeSmall)

    ColumnLayout {
        id: lines

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
}
