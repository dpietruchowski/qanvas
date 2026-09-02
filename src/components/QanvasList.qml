import Themed.Components

ThemedList {
    id: root

    property string textRole: ""
    property string subtitleRole: ""
    property string iconRole: ""
    property bool removable: false

    signal itemClicked(int index, var item)
    signal removeClicked(int index, var item)

    delegate: QanvasListItem {
        required property var modelData
        required property int index

        width: root.width
        title: root.textRole ? String(modelData[root.textRole]) : String(modelData)
        subtitle: root.subtitleRole ? String(modelData[root.subtitleRole]) : ""
        iconName: root.iconRole ? String(modelData[root.iconRole]) : ""
        removable: root.removable
        onClicked: root.itemClicked(index, modelData)
        onRemoveClicked: root.removeClicked(index, modelData)
    }
}
