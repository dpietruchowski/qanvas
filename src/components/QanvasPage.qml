import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedPage {
    id: root

    default property alias content: body.data
    property bool scrollable: false
    property bool showBack: false

    signal back()

    showBackButton: root.showBack

    onBackClicked: root.back()

    Flickable {
        id: flick

        anchors.fill: parent
        contentWidth: width
        contentHeight: root.scrollable ? body.implicitHeight : height
        interactive: root.scrollable
        clip: true

        ColumnLayout {
            id: body

            width: flick.width
            height: root.scrollable ? implicitHeight : flick.height
            spacing: Theme.page.contentSpacing
        }
    }
}
