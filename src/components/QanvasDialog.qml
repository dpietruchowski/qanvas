import Themed.Components

ThemedDialog {
    id: root

    property string type: ""
    property bool showCancel: false

    showRejectButton: root.showCancel
    dialogType: Theme.hasType(root.type, "danger") ? "error"
              : Theme.hasType(root.type, "success") ? "success"
              : "default"
}
