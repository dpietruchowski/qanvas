import QtQuick
import QtQuick.Layouts

import Themed.Components

RowLayout {
    id: root

    property string text: ""
    property string detail: ""
    property bool running: false
    property bool failed: false

    spacing: Theme.activity.markerSpacing

    Rectangle {
        objectName: root.objectName !== "" ? root.objectName + "Marker" : ""
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: Theme.activity.markerSize
        implicitHeight: Theme.activity.markerSize
        radius: Theme.activity.markerRadius
        color: root.failed ? Theme.activity.failedMarker
                           : root.running ? Theme.activity.runningMarker
                                          : Theme.activity.doneMarker

        SequentialAnimation on opacity {
            running: root.running
            loops: Animation.Infinite
            alwaysRunToEnd: true

            NumberAnimation { to: 0.35; duration: 520; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 1.0; duration: 520; easing.type: Easing.InOutQuad }
        }
    }

    Text {
        objectName: root.objectName !== "" ? root.objectName + "Text" : ""
        text: root.text
        color: root.running ? Theme.activity.runningText : Theme.activity.doneText
        font.pixelSize: Theme.activity.fontSize
        font.family: Theme.activity.fontFamily
    }

    Text {
        objectName: root.objectName !== "" ? root.objectName + "Detail" : ""
        Layout.fillWidth: true
        visible: root.detail !== ""
        text: root.detail
        elide: Text.ElideRight
        color: root.running ? Theme.activity.runningDetail : Theme.activity.doneDetail
        font.pixelSize: Theme.activity.fontSize
        font.family: Theme.activity.fontFamily
    }
}
