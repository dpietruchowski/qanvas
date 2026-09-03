import QtQuick
import QtQuick.Layouts

import Qanvas
import Themed.Components

Rectangle {
    id: root

    property var activities: []
    property bool busy: false
    property bool expanded: false

    readonly property var lastActivity: root.activities.length > 0
                                        ? root.activities[root.activities.length - 1]
                                        : null
    readonly property int doneCount: {
        var count = 0
        for (var i = 0; i < root.activities.length; ++i) {
            if (!root.activities[i].running)
                count += 1
        }
        return count
    }
    readonly property string statusText: {
        if (root.lastActivity)
            return root.lastActivity.text
        return root.busy ? qsTr("thinking") : qsTr("ask for a change")
    }

    signal sent(string text)

    function toggle() {
        root.expanded = !root.expanded
    }

    color: Theme.agentBar.background
    radius: Theme.agentBar.radius
    border.width: Theme.border.thin
    border.color: Theme.agentBar.border
    implicitHeight: layout.implicitHeight
    clip: true

    ColumnLayout {
        id: layout

        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.agentBar.headerHeight

            TapHandler {
                onTapped: root.toggle()
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.agentBar.padding
                anchors.rightMargin: Theme.spacing.xSmall
                spacing: Theme.agentBar.headerSpacing

                Item {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: Theme.agentBar.spinnerSize
                    implicitHeight: Theme.agentBar.spinnerSize

                    ThemedBusy {
                        objectName: "appAgentBusy"
                        anchors.centerIn: parent
                        size: Theme.agentBar.spinnerSize
                        dotSize: Theme.agentBar.spinnerDotSize
                        running: root.busy
                    }

                    ThemedIcon {
                        anchors.centerIn: parent
                        width: Theme.agentBar.spinnerSize
                        height: Theme.agentBar.spinnerSize
                        visible: !root.busy
                        svgSource: Theme.icons.aiApp
                        color: Theme.agentBar.idleIcon
                    }
                }

                Text {
                    objectName: "appAgentStatus"
                    Layout.fillWidth: true
                    text: root.statusText
                    elide: Text.ElideRight
                    color: root.busy ? Theme.agentBar.runningText : Theme.agentBar.idleText
                    font.pixelSize: Theme.agentBar.fontSize
                    font.family: Theme.agentBar.fontFamily
                }

                Text {
                    objectName: "appAgentCounter"
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.activities.length > 0
                    text: root.doneCount + "/" + root.activities.length
                    color: Theme.agentBar.counterText
                    font.pixelSize: Theme.agentBar.fontSize
                    font.family: Theme.agentBar.fontFamily
                }

                ThemedButton {
                    objectName: "appAgentToggle"
                    Layout.alignment: Qt.AlignVCenter
                    buttonSize: Theme.button.iconSmall
                    buttonStyle: Theme.button.ghost
                    iconSource: root.expanded ? Theme.icons.chevronUp : Theme.icons.chevronDown

                    onClicked: root.toggle()
                }
            }
        }

        Item {
            id: reveal

            property real revealedHeight: root.expanded
                                          ? content.implicitHeight + Theme.agentBar.padding
                                          : 0

            Layout.fillWidth: true
            Layout.preferredHeight: reveal.revealedHeight
            clip: true

            Behavior on revealedHeight {
                NumberAnimation {
                    duration: Theme.agentBar.expandDuration
                    easing.type: Easing.InOutQuad
                }
            }

            ColumnLayout {
                id: content

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: Theme.agentBar.padding
                anchors.rightMargin: Theme.agentBar.padding
                anchors.bottomMargin: Theme.agentBar.padding
                spacing: Theme.agentBar.logSpacing
                enabled: root.expanded

                ThemedScroll {
                    id: logScroll

                    objectName: "appAgentLogScroll"
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(log.implicitHeight,
                                                     Theme.agentBar.logMaxHeight)
                    visible: root.activities.length > 0

                    onContentHeightChanged: Qt.callLater(function () {
                        logScroll.contentY = Math.max(0, logScroll.contentHeight - logScroll.height)
                    })

                    ChatActivityLog {
                        id: log

                        objectName: "appAgentLog"
                        width: logScroll.width
                        activities: root.activities
                    }
                }

                ChatComposer {
                    objectName: "appAgentComposer"
                    Layout.fillWidth: true

                    onSent: function (text) { root.sent(text) }
                }
            }
        }
    }
}
