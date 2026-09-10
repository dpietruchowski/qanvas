import QtQuick
import QtQuick.Layouts

import Themed.Components

ThemedInput {
    id: root

    property string type: "text"

    Layout.fillWidth: true
    horizontalAlignment: Text.AlignLeft
    leftPadding: Theme.input.paddingH
    rightPadding: Theme.input.paddingH
    inputMethodHints: Theme.hasType(root.type, "number") ? Qt.ImhDigitsOnly
                                                         : Qt.ImhNoPredictiveText
    validator: Theme.hasType(root.type, "number") ? numberValidator : null

    IntValidator { id: numberValidator }
}
