import QtQuick

// The small caps heading each card carries.
Text {
    Theme { id: theme }

    color: theme.textFaint
    font.pixelSize: 11
    font.letterSpacing: 1.2
    font.capitalization: Font.AllUppercase
}
