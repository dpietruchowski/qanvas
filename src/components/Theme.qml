pragma Singleton
import QtQuick

QtObject {
    id: theme

    property int applicationWidth: 400
    property int applicationHeight: 800
    property int contentMaxWidth: 600
    property bool isNightMode: true

    property var colors: QtObject {
        property color background: "#17130F"
        property color surface: "#241F19"
        property color surfaceRaised: "#2C261F"
        property color surfaceSunken: "#120F0C"

        property color primary: "#E9A93C"
        property color primaryVariant: "#F2BC5C"
        property color primaryDark: "#CE9430"
        property color primarySurface: "#2E2417"

        property color border: "#2E2820"
        property color borderStrong: "#413930"
        property color divider: "#231E18"

        property color textPrimary: "#EDE7DF"
        property color textSecondary: "#A79E93"
        property color textMuted: "#8A8177"
        property color textPlaceholder: "#7A7167"
        property color textDisabled: "#5A534B"
        property color textInverse: "#17130F"

        property color cardBackground: "#241F19"
        property color cardBorder: "#2E2820"

        property color dialogSurface: "#221D17"
        property color overlay: "#000000B3"
        property color overlayLight: "#0000008C"

        property color danger: "#C4553D"
        property color dangerVariant: "#D46A52"
        property color dangerDark: "#A8462F"
        property color success: "#6E9E5B"
        property color successVariant: "#82B36E"
        property color successDark: "#5C8549"
    }

    property var fontSize: QtObject {
        property int xxLarge: 32
        property int xLarge: 24
        property int large: 20
        property int medium: 16
        property int normal: 14
        property int small: 12
        property int xSmall: 11
    }

    property var spacing: QtObject {
        property int xSmall: 4
        property int small: 8
        property int medium: 12
        property int large: 16
        property int xLarge: 24
    }

    property var padding: QtObject {
        property int xSmall: 4
        property int small: 8
        property int medium: 16
        property int large: 20
        property int xLarge: 28
        property int screen: 16
    }

    property var radius: QtObject {
        property int small: 6
        property int medium: 12
        property int large: 16
        property int xLarge: 22
        property int full: 9999
    }

    property var border: QtObject {
        property int thin: 1
        property int medium: 2
        property int thick: 3
    }

    property var panel: QtObject {
        property real maxHeightRatio: 0.9
        property int handleWidth: 38
        property int handleHeight: 4
        property int titleSize: theme.fontSize.large
    }

    property var navigation: QtObject {
        property int barHeight: 58
    }

    property var icon: QtObject {
        property int small: 16
        property int medium: 22
        property int large: 28
    }

    property var icons: QtObject {
        property string back: "qrc:/Themed/Icons/previous.svg"
        property string settings: "qrc:/Themed/Icons/sliders.svg"
        property string send: "qrc:/Themed/Icons/send.svg"
        property string chat: "qrc:/Themed/Icons/chat.svg"
        property string shelf: "qrc:/Themed/Icons/deck.svg"
        property string close: "qrc:/Themed/Icons/close.svg"
        property string app: "qrc:/Themed/Icons/ai-app.svg"
        property string remove: "qrc:/Themed/Icons/remove.svg"
        property string plus: "qrc:/Themed/Icons/plus.svg"
        property string check: "qrc:/Themed/Icons/check.svg"
        property string edit: "qrc:/Themed/Icons/edit.svg"
        property string search: "qrc:/Themed/Icons/search.svg"
    }

    property var card: QtObject {
        property int sizeSmall: 56
    }

    property var elevation: QtObject {
        property int shadowSteps: 3
        property int shadowBlur: 6
        property int shadowOffset: 2
        property color shadowColor: "#000000"
        property real shadowLayerOpacity: 0.14
    }

    property var appList: QtObject {
        property int spacing: theme.spacing.medium
        property int cardPadding: theme.padding.medium
        property int titleFontSize: theme.fontSize.medium
        property color titleColor: theme.colors.textPrimary
        property int subtitleFontSize: theme.fontSize.small
        property color subtitleColor: theme.colors.textMuted
        property int lineSpacing: theme.spacing.xSmall
        property int actionSpacing: theme.spacing.medium
        property int emptyFontSize: theme.fontSize.normal
        property color emptyColor: theme.colors.textMuted
    }

    property var appStage: QtObject {
        property color background: theme.colors.surfaceSunken
        property color border: theme.colors.border
        property int radius: theme.radius.medium
        property color placeholderText: theme.colors.textMuted
        property int placeholderFontSize: theme.fontSize.normal
        property int placeholderPadding: theme.padding.large
    }

    property var chat: QtObject {
        property real bubbleMaxWidthRatio: 0.78
        property int bubbleRadius: theme.radius.large
        property int bubblePaddingH: 14
        property int bubblePaddingV: 10
        property int fontSize: theme.fontSize.normal
        property int lineSpacing: 4
        property int messageSpacing: theme.spacing.large
        property int composerHeight: 52
        property int headerHeight: 48
    }

    property var activity: QtObject {
        property int spacing: theme.spacing.small
        property int markerSpacing: theme.spacing.small
        property int markerSize: 8
        property int markerRadius: 2
        property int fontSize: theme.fontSize.small
        property string fontFamily: "monospace"
        property int tickInterval: 1000
        property color runningMarker: theme.colors.primary
        property color doneMarker: theme.colors.textDisabled
        property color failedMarker: theme.colors.danger
        property color runningText: theme.colors.textPrimary
        property color doneText: theme.colors.textDisabled
        property color runningDetail: theme.colors.primary
        property color doneDetail: theme.colors.textSecondary
    }

    property var chatBubble: QtObject {
        property var user: QtObject {
            property color background: theme.colors.surface
            property color border: "transparent"
            property color text: theme.colors.textPrimary
            property int radius: theme.radius.large
            property int paddingH: 14
            property int paddingV: 10
            property int alignment: Qt.AlignRight
        }

        property var agent: QtObject {
            property color background: "transparent"
            property color border: "transparent"
            property color text: theme.colors.textPrimary
            property int radius: 0
            property int paddingH: 0
            property int paddingV: 0
            property int alignment: Qt.AlignLeft
        }
    }

    property var button: QtObject {
        property int radius: theme.radius.medium

        property var icon: QtObject {
            property int size: 40
            property int width: size
            property int height: size
            property int fontSize: theme.fontSize.small
            property int iconSize: theme.icon.medium
        }

        property var circle: QtObject {
            property int size: 44
            property int width: size
            property int height: size
            property int fontSize: theme.fontSize.normal
            property int iconSize: theme.icon.medium
        }

        property var small: QtObject {
            property int width: theme.applicationWidth * 0.24
            property int height: 34
            property int fontSize: theme.fontSize.small
            property int iconSize: theme.icon.small
        }

        property var medium: QtObject {
            property int width: theme.applicationWidth * 0.32
            property int height: 44
            property int fontSize: theme.fontSize.normal
            property int iconSize: theme.icon.medium
        }

        property var large: QtObject {
            property int width: theme.applicationWidth * 0.5
            property int height: 52
            property int fontSize: theme.fontSize.medium
            property int iconSize: theme.icon.large
        }

        property var primary: QtObject {
            property color background: theme.colors.primary
            property color hovered: theme.colors.primaryVariant
            property color pressed: theme.colors.primaryDark
            property color border: theme.colors.primary
            property color text: theme.colors.textInverse
        }

        property var subtle: QtObject {
            property color background: theme.colors.surface
            property color hovered: theme.colors.surfaceRaised
            property color pressed: theme.colors.surfaceSunken
            property color border: theme.colors.border
            property color text: theme.colors.textSecondary
        }

        property var accent: QtObject {
            property color background: theme.colors.primarySurface
            property color hovered: "#3A2D1C"
            property color pressed: "#251D12"
            property color border: "#00000000"
            property color text: theme.colors.primary
        }

        property var ghost: QtObject {
            property color background: "#00000000"
            property color hovered: "#14FFFFFF"
            property color pressed: "#0AFFFFFF"
            property color border: "#00000000"
            property color text: theme.colors.textSecondary
        }

        property var secondary: QtObject {
            property color background: theme.colors.surface
            property color hovered: theme.colors.surfaceRaised
            property color pressed: theme.colors.surfaceSunken
            property color border: theme.colors.border
            property color text: theme.colors.textPrimary
        }

        property var danger: QtObject {
            property color background: theme.colors.danger
            property color hovered: theme.colors.dangerVariant
            property color pressed: theme.colors.dangerDark
            property color border: theme.colors.danger
            property color text: theme.colors.textPrimary
        }

        property var success: QtObject {
            property color background: theme.colors.success
            property color hovered: theme.colors.successVariant
            property color pressed: theme.colors.successDark
            property color border: theme.colors.success
            property color text: theme.colors.textPrimary
        }
    }

    property var input: QtObject {
        property color background: theme.colors.surface
        property color border: theme.colors.border
        property color borderFocused: theme.colors.borderStrong
        property int radius: theme.radius.xLarge
        property int height: theme.chat.composerHeight
        property int fontSize: theme.fontSize.normal
        property int paddingH: theme.padding.medium
    }

    property var settings: QtObject {
        property color labelColor: theme.colors.textMuted
        property int labelSize: theme.fontSize.xSmall
        property real labelSpacing: 1.4
        property int labelBottomSpacing: theme.spacing.small
        property int sectionSpacing: theme.spacing.xLarge
        property int optionSpacing: theme.spacing.small
        property color fieldBackground: theme.colors.background
        property color fieldBorder: theme.colors.border
        property color fieldBorderFocused: theme.colors.borderStrong
        property int fieldHeight: 48
        property int fieldRadius: theme.radius.medium
        property int fieldFontSize: theme.fontSize.normal
        property int fieldPadding: theme.padding.medium
        property string fontFamily: Qt.application.font.family
        property string monoFamily: "monospace"
    }

    property var text: QtObject {
        property var title: QtObject {
            property int fontSize: theme.fontSize.xLarge
            property color color: theme.colors.textPrimary
            property bool bold: true
            property real lineHeight: 1.0
        }

        property var subtitle: QtObject {
            property int fontSize: theme.fontSize.medium
            property color color: theme.colors.textPrimary
            property bool bold: true
            property real lineHeight: 1.0
        }

        property var body: QtObject {
            property int fontSize: theme.fontSize.normal
            property color color: theme.colors.textPrimary
            property bool bold: false
            property real lineHeight: 1.15
        }

        property var caption: QtObject {
            property int fontSize: theme.fontSize.small
            property color color: theme.colors.textSecondary
            property bool bold: false
            property real lineHeight: 1.15
        }

        property var muted: QtObject {
            property int fontSize: theme.fontSize.normal
            property color color: theme.colors.textMuted
            property bool bold: false
            property real lineHeight: 1.15
        }
    }

    property var page: QtObject {
        property var title: theme.text.title
        property int contentSpacing: theme.spacing.medium
    }

    property var section: QtObject {
        property int spacing: theme.spacing.small
        property int contentSpacing: theme.spacing.small
    }

    property var separator: QtObject {
        property int thickness: theme.border.thin
        property color color: theme.colors.divider
    }

    property var list: QtObject {
        property int spacing: theme.spacing.small
        property int itemHeight: 56
        property int itemPadding: theme.padding.medium
        property int iconSize: theme.icon.medium
        property color iconColor: theme.colors.textSecondary
        property int lineSpacing: theme.spacing.xSmall
    }

    property var grid: QtObject {
        property int columns: 2
        property int spacing: theme.spacing.small
    }

    property var scroll: QtObject {
        property int barWidth: 6
        property int barRadius: theme.radius.small
        property color barColor: theme.colors.borderStrong
        property real barIdleOpacity: 0.4
    }

    property var empty: QtObject {
        property int spacing: theme.spacing.medium
        property int iconSize: theme.icon.large
        property color iconColor: theme.colors.textMuted
        property int maxWidth: 280
    }

    property var slider: QtObject {
        property int width: 200
        property int trackHeight: 6
        property int handleSize: 22
        property color trackColor: theme.colors.divider
        property color fillColor: theme.colors.primary
        property color handleColor: theme.colors.surfaceRaised
        property color handlePressedColor: theme.colors.primarySurface
    }

    property var busy: QtObject {
        property int size: 32
        property int dotSize: 5
        property int dotCount: 8
        property int duration: 900
        property color color: theme.colors.primary
    }

    function iconSource(name) {
        return name ? "qrc:/Themed/Icons/" + name + ".svg" : ""
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

    function textStyle(type) {
        if (theme.hasType(type, "title"))
            return theme.text.title
        if (theme.hasType(type, "subtitle"))
            return theme.text.subtitle
        if (theme.hasType(type, "caption"))
            return theme.text.caption
        if (theme.hasType(type, "muted"))
            return theme.text.muted
        return theme.text.body
    }
}
