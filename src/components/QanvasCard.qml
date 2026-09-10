import QtQuick.Layouts

import Themed.Components

ThemedCard {
    default property alias cardContent: column.data

    Layout.fillWidth: true
    margins: Theme.padding.medium

    content: ColumnLayout {
        id: column

        spacing: Theme.spacing.small
    }
}
