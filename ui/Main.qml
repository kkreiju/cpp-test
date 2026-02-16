import QtQuick
import QtQuick.Controls
import QtQuick.Window

/**
 * Main.qml - Root application window for NCTV Player.
 * Replaces index.html & app.ts from the Electron build.
 * Loads the Splash screen first, then transitions to PlayerLayout.
 */
ApplicationWindow {
    id: root

    // Fullscreen, borderless window
    visibility: Window.FullScreen
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "black"
    title: "NCTV Player"

    width: 1920
    height: 1080

    // ── State Machine ──
    // "splash"  → Splash screen visible (during startup / optimization)
    // "player"  → Player layout visible (normal operation)
    // "menu"    → Settings / debug menu overlay
    property string appState: "splash"

    // ── Splash Screen ──
    Loader {
        id: splashLoader
        anchors.fill: parent
        active: root.appState === "splash"
        source: "qrc:/ui/features/splash/Splash.qml"
        z: 100   // Always on top while active
    }

    // ── Player Layout (zones) ──
    Loader {
        id: playerLoader
        anchors.fill: parent
        active: root.appState === "player"
        source: "qrc:/ui/features/player/PlayerLayout.qml"
        z: 0
    }

    // ── Menu Overlay ──
    Loader {
        id: menuLoader
        anchors.fill: parent
        active: root.appState === "menu"
        source: "qrc:/ui/features/menu/Menu.qml"
        z: 200
    }

    // ── Connections ──

    // Transition from splash → player when optimization is done
    Connections {
        target: videoOptimizer
        function onOptimizationFinished() {
            console.log("[Main.qml] Optimization finished, switching to player view");
            root.appState = "player";
        }
    }

    // Keyboard shortcuts
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (root.appState === "menu") {
                root.appState = "player";
            } else {
                Qt.quit();
            }
        }
    }

    Shortcut {
        sequence: "F12"
        onActivated: {
            root.appState = (root.appState === "menu") ? "player" : "menu";
        }
    }

    Component.onCompleted: {
        console.log("[Main.qml] Application window loaded (" + width + "x" + height + ")");
        windowService.setMainWindow(root);
    }
}
