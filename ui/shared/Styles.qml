pragma Singleton
import QtQuick

/**
 * Styles.qml - Centralized style definitions.
 * Replaces styles.css from the Electron build.
 * Import as: import "../shared" and use Styles.primaryColor etc.
 */
QtObject {
    // ── Colors ──
    readonly property color primaryColor:     "#e94560"
    readonly property color secondaryColor:   "#1a1a2e"
    readonly property color backgroundColor:  "#0f0f23"
    readonly property color surfaceColor:     "#16213e"
    readonly property color textColor:        "#ffffff"
    readonly property color textMuted:        "#a0a0b0"
    readonly property color accentColor:      "#0f3460"
    readonly property color successColor:     "#4caf50"
    readonly property color warningColor:     "#ff9800"
    readonly property color errorColor:       "#f44336"

    // ── Typography ──
    readonly property int fontSizeSmall:   12
    readonly property int fontSizeMedium:  16
    readonly property int fontSizeLarge:   24
    readonly property int fontSizeXLarge:  36
    readonly property int fontSizeTitle:   48

    // ── Spacing ──
    readonly property int spacingSmall:    8
    readonly property int spacingMedium:   16
    readonly property int spacingLarge:    24
    readonly property int spacingXLarge:   32

    // ── Zone Definitions (1920x1080) ──
    readonly property var zones: ({
        background: { x: 0,    y: 0,   width: 1920, height: 1080, z: 0 },
        main:       { x: 0,    y: 21,  width: 1472, height: 828,  z: 1 },
        horizontal: { x: 0,    y: 870, width: 1920, height: 189,  z: 1 },
        vertical:   { x: 1472, y: 21,  width: 448,  height: 849,  z: 1 }
    })

    // ── Animation Durations ──
    readonly property int animFast:   200
    readonly property int animMedium: 400
    readonly property int animSlow:   800
}
