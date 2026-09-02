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
        property int dotSize: 7
        property int dotSpacing: theme.spacing.small
        property int composerHeight: 52
        property int headerHeight: 48
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
}
