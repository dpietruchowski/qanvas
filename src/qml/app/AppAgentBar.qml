import QtQuick
import QtQuick.Layouts

import Chat.Components
import Themed.Components

Rectangle {
    id: root

    property var messages: null
    property string activity: ""
    property bool busy: false
    property bool locked: false
    property bool expanded: false

    readonly property string statusText: {
        if (root.activity !== "")
            return root.activity
        return root.busy ? qsTr("thinking") : qsTr("ask for a change")
    }

    signal sent(string text)
    signal stopRequested()
    signal retryRequested()
    signal backClicked()

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
                anchors.leftMargin: Theme.spacing.xSmall
                anchors.rightMargin: Theme.spacing.xSmall
                spacing: Theme.agentBar.headerSpacing

                ThemedBackButton {
                    objectName: "appAgentBack"
                    Layout.alignment: Qt.AlignVCenter
                    buttonSize: Theme.button.iconSmall

                    onClicked: root.backClicked()
                }

                ThemedSeparator {
                    objectName: "appAgentDivider"
                    Layout.fillHeight: true
                    Layout.topMargin: Theme.spacing.small
                    Layout.bottomMargin: Theme.spacing.small
                    vertical: true
                    color: Theme.agentBar.divider
                }

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
                visible: reveal.revealedHeight > 0

                ChatMessageList {
                    objectName: "appAgentMessages"
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(contentHeight, Theme.agentBar.logMaxHeight)
                    visible: count > 0 || root.busy
                    model: root.messages
                    busy: root.busy
                    activity: root.activity

                    onRetryRequested: root.retryRequested()
                }

                ChatComposer {
                    objectName: "appAgentComposer"
                    Layout.fillWidth: true
                    enabled: !root.locked
                    busy: root.busy

                    onSent: function (text) { root.sent(text) }
                    onStopRequested: root.stopRequested()
                }
            }
        }
    }
}
