import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedList {
    id: root

    Layout.fillWidth: true

    property string textRole: ""
    property string subtitleRole: ""
    property string iconRole: ""
    property bool removable: false
    property var textFormat: null
    property var subtitleFormat: null
    property string emptyText: ""
    property string emptyIcon: ""

    signal itemClicked(int index, var item)
    signal removeClicked(int index, var item)

    function formatted(hook, role, item) {
        if (hook)
            return String(hook(item))
        if (role)
            return String(item[role])
        return ""
    }

    implicitHeight: empty.visible ? empty.implicitHeight : contentHeight

    delegate: QanvasListItem {
        required property var modelData
        required property int index

        width: root.width
        title: root.textFormat || root.textRole
               ? root.formatted(root.textFormat, root.textRole, modelData)
               : String(modelData)
        subtitle: root.formatted(root.subtitleFormat, root.subtitleRole, modelData)
        iconName: root.formatted(null, root.iconRole, modelData)
        removable: root.removable
        onClicked: root.itemClicked(index, modelData)
        onRemoveClicked: root.removeClicked(index, modelData)
    }

    ThemedEmpty {
        id: empty

        parent: root
        anchors.centerIn: parent
        width: parent.width
        visible: root.count === 0 && root.emptyText !== ""
        text: root.emptyText
        iconSource: Theme.iconSource(root.emptyIcon)
    }
}
