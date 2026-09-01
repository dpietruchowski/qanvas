import QtQuick

// One place for the app's colours and metrics. Not a singleton on purpose: that
// would need a qmldir, and a generated app has no reason to carry one. Each
// component instantiates its own — they are a handful of constants.
QtObject {
    readonly property color background: "#0e1621"
    readonly property color surface: "#16212e"
    readonly property color surfaceRaised: "#22303f"
    readonly property color line: "#27384a"
    readonly property color accent: "#4c9aff"
    readonly property color accentText: "#0b1220"
    readonly property color text: "#e6edf3"
    readonly property color textDim: "#8b9bad"
    readonly property color textFaint: "#5d6d7e"

    readonly property int radius: 10
    readonly property int gap: 12
    readonly property int pad: 16
}
