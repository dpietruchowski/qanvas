pragma Singleton
import QtQuick
import Themed.Theme

DefaultTheme {
    id: theme

    isNightMode: true
    contentMaxWidthBase: 600

    colors: QtObject {
        readonly property color backgroundDark: "#17130F"
        readonly property color backgroundLight: "#17130F"
        readonly property color background: "#17130F"
        readonly property color surface: "#241F19"
        readonly property color surfaceRaised: "#2C261F"
        readonly property color surfaceSunken: "#120F0C"
        readonly property color dialogSurface: "#221D17"

        readonly property color primary: "#E9A93C"
        readonly property color primaryVariant: "#F2BC5C"
        readonly property color primaryDark: "#CE9430"
        readonly property color primarySurface: "#2E2417"
        readonly property color secondary: "#2C261F"
        readonly property color accent: "#E9A93C"

        readonly property color info: "#E9A93C"
        readonly property color success: "#6E9E5B"
        readonly property color successVariant: "#82B36E"
        readonly property color successDark: "#5C8549"
        readonly property color warning: "#FFA826"
        readonly property color error: "#C4553D"
        readonly property color errorVariant: "#D46A52"
        readonly property color errorDark: "#A8462F"

        readonly property color textPrimary: "#EDE7DF"
        readonly property color textSecondary: "#A79E93"
        readonly property color textMuted: "#8A8177"
        readonly property color textPlaceholder: "#7A7167"
        readonly property color textDisabled: "#5A534B"
        readonly property color textInverse: "#17130F"

        readonly property color border: "#2E2820"
        readonly property color borderStrong: "#413930"
        readonly property color divider: "#231E18"
        readonly property color overlay: "#000000B3"
        readonly property color overlayLight: "#0000008C"

        readonly property color cardBackground: "#241F19"
        readonly property color cardBorder: "#2E2820"
        readonly property color cardBorderHover: "#413930"
    }

    page: QtObject {
        readonly property QtObject title: QtObject {
            readonly property color color: theme.colors.textPrimary
            readonly property int fontSize: theme.fontSize.xLarge
            readonly property bool bold: true
            readonly property real lineHeight: 1.0
        }
        readonly property int contentSpacing: theme.spacing.medium
    }

    grid: QtObject {
        readonly property int columns: theme.isLandscape ? 3 : 2
        readonly property int spacing: theme.spacing.small
    }

    button: QtObject {
        readonly property int radius: theme.radius.medium

        readonly property QtObject square: QtObject {
            readonly property int size: theme.scaled(32)
            readonly property int width: size
            readonly property int height: size
            readonly property int fontSize: theme.fontSize.small
            readonly property int iconSize: theme.scaled(18)
        }

        readonly property QtObject icon: QtObject {
            readonly property int size: theme.scaled(40)
            readonly property int width: size
            readonly property int height: size
            readonly property int fontSize: theme.fontSize.small
            readonly property int iconSize: theme.icon.medium
        }

        readonly property QtObject iconSmall: QtObject {
            readonly property int size: theme.scaled(33)
            readonly property int width: size
            readonly property int height: size
            readonly property int fontSize: theme.fontSize.normal
            readonly property int iconSize: theme.icon.small
        }

        readonly property QtObject circle: QtObject {
            readonly property int size: theme.scaled(44)
            readonly property int width: size
            readonly property int height: size
            readonly property int fontSize: theme.fontSize.normal
            readonly property int iconSize: theme.icon.medium
        }

        readonly property QtObject small: QtObject {
            readonly property int width: theme.contentWidth * 0.24
            readonly property int height: theme.scaled(34)
            readonly property int fontSize: theme.fontSize.small
            readonly property int iconSize: theme.icon.small
        }

        readonly property QtObject compact: QtObject {
            readonly property int width: theme.contentWidth * 0.24
            readonly property int height: theme.scaled(40)
            readonly property int fontSize: theme.fontSize.normal
            readonly property int iconSize: theme.scaled(18)
        }

        readonly property QtObject medium: QtObject {
            readonly property int width: theme.contentWidth * 0.32
            readonly property int height: theme.scaled(44)
            readonly property int fontSize: theme.fontSize.normal
            readonly property int iconSize: theme.icon.medium
        }

        readonly property QtObject large: QtObject {
            readonly property int width: theme.contentWidth * 0.5
            readonly property int height: theme.scaled(52)
            readonly property int fontSize: theme.fontSize.medium
            readonly property int iconSize: theme.icon.large
        }

        readonly property QtObject row: QtObject {
            readonly property int width: theme.contentWidth
            readonly property int height: theme.scaled(52)
            readonly property int fontSize: theme.fontSize.medium
            readonly property int iconSize: theme.icon.medium
        }

        readonly property QtObject rowCompact: QtObject {
            readonly property int width: theme.contentWidth
            readonly property int height: theme.scaled(44)
            readonly property int fontSize: theme.fontSize.normal
            readonly property int iconSize: theme.scaled(18)
        }

        readonly property QtObject primary: QtObject {
            readonly property color background: theme.colors.primary
            readonly property color hovered: theme.colors.primaryVariant
            readonly property color pressed: theme.colors.primaryDark
            readonly property color border: theme.colors.primary
            readonly property color text: theme.colors.textInverse
        }

        readonly property QtObject primarySoft: QtObject {
            readonly property color background: theme.colors.primarySurface
            readonly property color hovered: "#3A2D1C"
            readonly property color pressed: "#251D12"
            readonly property color border: "transparent"
            readonly property color text: theme.colors.primary
        }

        readonly property QtObject accent: primarySoft

        readonly property QtObject secondary: QtObject {
            readonly property color background: theme.colors.surface
            readonly property color hovered: theme.colors.surfaceRaised
            readonly property color pressed: theme.colors.surfaceSunken
            readonly property color border: theme.colors.border
            readonly property color text: theme.colors.textPrimary
        }

        readonly property QtObject secondarySoft: QtObject {
            readonly property color background: "transparent"
            readonly property color hovered: theme.colors.surface
            readonly property color pressed: theme.colors.surfaceSunken
            readonly property color border: theme.colors.border
            readonly property color text: theme.colors.textSecondary
        }

        readonly property QtObject subtle: QtObject {
            readonly property color background: theme.colors.surface
            readonly property color hovered: theme.colors.surfaceRaised
            readonly property color pressed: theme.colors.surfaceSunken
            readonly property color border: theme.colors.border
            readonly property color text: theme.colors.textSecondary
        }

        readonly property QtObject success: QtObject {
            readonly property color background: theme.colors.success
            readonly property color hovered: theme.colors.successVariant
            readonly property color pressed: theme.colors.successDark
            readonly property color border: theme.colors.success
            readonly property color text: theme.colors.textPrimary
        }

        readonly property QtObject danger: QtObject {
            readonly property color background: theme.colors.error
            readonly property color hovered: theme.colors.errorVariant
            readonly property color pressed: theme.colors.errorDark
            readonly property color border: theme.colors.error
            readonly property color text: theme.colors.textPrimary
        }

        readonly property QtObject dangerSoft: QtObject {
            readonly property color background: Qt.rgba(theme.colors.error.r, theme.colors.error.g, theme.colors.error.b, 0.24)
            readonly property color hovered: Qt.rgba(theme.colors.error.r, theme.colors.error.g, theme.colors.error.b, 0.34)
            readonly property color pressed: Qt.rgba(theme.colors.error.r, theme.colors.error.g, theme.colors.error.b, 0.42)
            readonly property color border: Qt.rgba(theme.colors.error.r, theme.colors.error.g, theme.colors.error.b, 0.60)
            readonly property color text: theme.colors.error
        }

        readonly property QtObject warning: QtObject {
            readonly property color background: theme.colors.warning
            readonly property color hovered: "#FFB84D"
            readonly property color pressed: "#E69500"
            readonly property color border: theme.colors.warning
            readonly property color text: theme.colors.textInverse
        }

        readonly property QtObject ghost: QtObject {
            readonly property color background: "transparent"
            readonly property color hovered: "#14FFFFFF"
            readonly property color pressed: "#0AFFFFFF"
            readonly property color border: "transparent"
            readonly property color text: theme.colors.textSecondary
        }

        readonly property QtObject ghostPrimary: QtObject {
            readonly property color background: "transparent"
            readonly property color hovered: Qt.rgba(theme.colors.primary.r, theme.colors.primary.g, theme.colors.primary.b, 0.16)
            readonly property color pressed: Qt.rgba(theme.colors.primary.r, theme.colors.primary.g, theme.colors.primary.b, 0.26)
            readonly property color border: theme.colors.primary
            readonly property color text: theme.colors.primary
        }

        readonly property QtObject ghostDanger: QtObject {
            readonly property color background: "transparent"
            readonly property color hovered: Qt.rgba(theme.colors.error.r, theme.colors.error.g, theme.colors.error.b, 0.16)
            readonly property color pressed: Qt.rgba(theme.colors.error.r, theme.colors.error.g, theme.colors.error.b, 0.26)
            readonly property color border: Qt.rgba(theme.colors.error.r, theme.colors.error.g, theme.colors.error.b, 0.60)
            readonly property color text: theme.colors.error
        }
    }

    property QtObject appList: QtObject {
        readonly property int spacing: theme.spacing.medium
        readonly property int cardPadding: theme.padding.medium
        readonly property int cardMinHeight: theme.scaled(56)
        readonly property int titleFontSize: theme.fontSize.medium
        readonly property color titleColor: theme.colors.textPrimary
        readonly property int subtitleFontSize: theme.fontSize.small
        readonly property color subtitleColor: theme.colors.textMuted
        readonly property int lineSpacing: theme.spacing.xSmall
        readonly property int actionSpacing: theme.spacing.medium
        readonly property int emptyFontSize: theme.fontSize.normal
        readonly property color emptyColor: theme.colors.textMuted
    }

    property QtObject appStage: QtObject {
        readonly property color background: theme.colors.surfaceSunken
        readonly property color border: theme.colors.border
        readonly property int radius: theme.radius.medium
        readonly property color placeholderText: theme.colors.textMuted
        readonly property int placeholderFontSize: theme.fontSize.normal
        readonly property int placeholderPadding: theme.padding.large
    }

    property QtObject agentBar: QtObject {
        readonly property color background: theme.colors.surfaceRaised
        readonly property color border: theme.colors.border
        readonly property color divider: theme.colors.borderStrong
        readonly property int radius: theme.radius.large
        readonly property int padding: theme.padding.medium
        readonly property int headerHeight: theme.scaled(44)
        readonly property int headerSpacing: theme.spacing.small
        readonly property int spinnerSize: theme.scaled(16)
        readonly property int spinnerDotSize: theme.scaled(3)
        readonly property int iconSize: theme.icon.small
        readonly property color idleIcon: theme.colors.textMuted
        readonly property int fontSize: theme.fontSize.small
        readonly property string fontFamily: "monospace"
        readonly property color runningText: theme.colors.textPrimary
        readonly property color idleText: theme.colors.textMuted
        readonly property int logMaxHeight: theme.scaled(200)
        readonly property int logSpacing: theme.spacing.medium
        readonly property int expandDuration: 180
    }

    property QtObject input: QtObject {
        readonly property color background: theme.colors.surface
        readonly property color border: theme.colors.border
        readonly property color borderFocused: theme.colors.borderStrong
        readonly property int radius: theme.radius.xLarge
        readonly property int height: theme.chat.composerHeight
        readonly property int fontSize: theme.fontSize.normal
        readonly property int paddingH: theme.padding.medium
    }

    property QtObject settings: QtObject {
        readonly property color labelColor: theme.colors.textMuted
        readonly property int labelSize: theme.fontSize.xSmall
        readonly property real labelSpacing: 1.4
        readonly property int labelBottomSpacing: theme.spacing.small
        readonly property int sectionSpacing: theme.spacing.xLarge
        readonly property int optionSpacing: theme.spacing.small
        readonly property color fieldBackground: theme.colors.background
        readonly property color fieldBorder: theme.colors.border
        readonly property color fieldBorderFocused: theme.colors.borderStrong
        readonly property int fieldHeight: theme.scaled(48)
        readonly property int fieldRadius: theme.radius.medium
        readonly property int fieldFontSize: theme.fontSize.normal
        readonly property int fieldPadding: theme.padding.medium
        readonly property string fontFamily: Qt.application.font.family
        readonly property string monoFamily: "monospace"
    }

    readonly property var iconNames: [
        "plus", "minus", "remove", "check", "close", "edit", "copy", "search", "settings",
        "list", "menu", "star", "star-half", "star-full", "timer", "calendar", "home", "user",
        "info", "warning", "error", "success", "chat", "send", "globe", "next", "previous",
        "chevron-down", "chevron-up", "chevron-right", "barbell", "dumbell", "sliders",
        "more-vertical", "reorder", "export", "import"
    ]

    readonly property var iconAliases: ({
        "add": "plus", "delete": "remove", "trash": "remove", "ok": "check", "done": "check",
        "cancel": "close", "x": "close", "back": "previous", "forward": "next",
        "clock": "timer", "history": "timer", "stopwatch": "timer", "gear": "settings",
        "cog": "settings", "help": "info", "dumbbell": "dumbell", "gym": "barbell",
        "workout": "dumbell", "stats": "sliders", "chart": "sliders", "filter": "sliders",
        "more": "more-vertical", "drag": "reorder", "share": "export", "download": "import",
        "date": "calendar", "person": "user", "profile": "user", "message": "chat",
        "book": "list", "notes": "list", "heart": "star-full", "favorite": "star-full",
        "bookmark": "star", "lightbulb": "info", "question": "info", "alert": "warning",
        "play": "next", "trophy": "success", "quiz": "check", "cards": "copy"
    })

    function iconSource(name) {
        if (!name)
            return ""
        const resolved = theme.iconAliases[name] || name
        if (theme.iconNames.indexOf(resolved) === -1)
            throw new Error("unknown icon \"" + name + "\": iconName is one of "
                            + theme.iconNames.join(" "))
        return "qrc:/Themed/Icons/" + resolved + ".svg"
    }

    function hasType(type, token) {
        return (" " + String(type).toLowerCase() + " ").indexOf(" " + token + " ") !== -1
    }

    function buttonStyle(type) {
        if (theme.hasType(type, "danger"))
            return theme.button.danger
        if (theme.hasType(type, "success"))
            return theme.button.success
        if (theme.hasType(type, "secondary"))
            return theme.button.secondary
        if (theme.hasType(type, "ghost"))
            return theme.button.ghost
        return theme.button.primary
    }

    function buttonSize(type) {
        if (theme.hasType(type, "circle"))
            return theme.button.circle
        if (theme.hasType(type, "small"))
            return theme.button.small
        if (theme.hasType(type, "large"))
            return theme.button.large
        return theme.button.medium
    }

    function textAlignment(type) {
        if (theme.hasType(type, "center"))
            return Text.AlignHCenter
        if (theme.hasType(type, "right"))
            return Text.AlignRight
        return Text.AlignLeft
    }

    function badgeStyle(type) {
        if (theme.hasType(type, "danger"))
            return theme.badge.danger
        if (theme.hasType(type, "success"))
            return theme.badge.success
        if (theme.hasType(type, "warning"))
            return theme.badge.warning
        if (theme.hasType(type, "primary"))
            return theme.badge.primary
        return theme.badge.neutral
    }

    function statValueStyle(type) {
        if (theme.hasType(type, "danger"))
            return theme.statDanger
        if (theme.hasType(type, "success"))
            return theme.statSuccess
        if (theme.hasType(type, "primary"))
            return theme.statPrimary
        return theme.stat.value
    }

    readonly property QtObject statPrimary: QtObject {
        readonly property color color: theme.colors.primary
        readonly property int fontSize: theme.stat.value.fontSize
        readonly property bool bold: true
        readonly property real lineHeight: 1.1
    }

    readonly property QtObject statSuccess: QtObject {
        readonly property color color: theme.colors.success
        readonly property int fontSize: theme.stat.value.fontSize
        readonly property bool bold: true
        readonly property real lineHeight: 1.1
    }

    readonly property QtObject statDanger: QtObject {
        readonly property color color: theme.colors.error
        readonly property int fontSize: theme.stat.value.fontSize
        readonly property bool bold: true
        readonly property real lineHeight: 1.1
    }

    function textStyle(type) {
        if (theme.hasType(type, "display"))
            return theme.text.display
        if (theme.hasType(type, "title"))
            return theme.page.title
        if (theme.hasType(type, "subtitle"))
            return theme.text.subtitle
        if (theme.hasType(type, "caption"))
            return theme.text.caption
        if (theme.hasType(type, "muted"))
            return theme.text.muted
        return theme.text.body
    }
}
