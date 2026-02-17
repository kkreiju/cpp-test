import QtQuick
import QtQuick.Controls

/**
 * Splash.qml - Boot splash screen displayed during startup and video optimization.
 * Replaces splash.component from the Electron build.
 */
Rectangle {
    id: splashRoot
    anchors.fill: parent
    color: "#1a1a2e"
    z: 100

    // ── Animated Logo ──
    Column {
        anchors.centerIn: parent
        spacing: 30

        // Logo / Brand
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "NCTV"
            font.pixelSize: 72
            font.weight: Font.Bold
            font.letterSpacing: 12
            color: "#8dcb2c"

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.6; to: 1.0; duration: 1200; easing.type: Easing.InOutSine }
                NumberAnimation { from: 1.0; to: 0.6; duration: 1200; easing.type: Easing.InOutSine }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "DIGITAL SIGNAGE PLAYER"
            font.pixelSize: 18
            font.letterSpacing: 6
            color: "#a0a0b0"
        }

        // Loading Indicator
        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 200
            height: 4

            Rectangle {
                id: progressTrack
                anchors.fill: parent
                color: "#2a2a3e"
                radius: 2
            }

            Rectangle {
                id: progressBar
                height: parent.height
                radius: 2
                color: "#8dcb2c"

                SequentialAnimation on width {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: 200; duration: 2000; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 200; to: 0; duration: 0 }
                }
            }
        }

        // Status text bound to the optimizer
        Text {
            id: statusText
            anchors.horizontalCenter: parent.horizontalCenter
            text: typeof videoOptimizer !== "undefined" ? videoOptimizer.statusMessage : "Initializing..."
            font.pixelSize: 14
            color: "#707080"
        }
    }

    // Version stamp
    Text {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 30
        text: "v" + Qt.application.version
        font.pixelSize: 12
        color: "#404050"
    }
}
