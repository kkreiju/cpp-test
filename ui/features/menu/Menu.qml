import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Menu.qml - Debug / Settings overlay menu.
 * Toggled via F12 keyboard shortcut.
 * Replaces menu.component from the Electron build.
 */
Rectangle {
    id: menuRoot
    anchors.fill: parent
    color: "#DD000000"
    z: 200

    // Close on background click
    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.appState = "player";
        }
    }

    // Menu Panel
    Rectangle {
        anchors.centerIn: parent
        width: 600
        height: 500
        radius: 12
        color: "#1a1a2e"
        border.color: "#e94560"
        border.width: 2

        // Prevent click-through
        MouseArea { anchors.fill: parent }

        Column {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 16

            // Title
            Text {
                text: "NCTV Player Settings"
                font.pixelSize: 24
                font.bold: true
                color: "#e94560"
            }

            Rectangle { width: parent.width; height: 1; color: "#333" }

            // Config info
            Text {
                text: "Configuration"
                font.pixelSize: 16
                font.bold: true
                color: "white"
            }

            GridLayout {
                columns: 2
                columnSpacing: 20
                rowSpacing: 8

                Text { color: "#aaa"; text: "Kiosk Mode:" }
                Text { color: "white"; text: appConfig.kioskMode ? "Enabled" : "Disabled" }

                Text { color: "#aaa"; text: "Image Duration:" }
                Text { color: "white"; text: appConfig.imageDurationMs + " ms" }

                Text { color: "#aaa"; text: "Retry Interval:" }
                Text { color: "white"; text: appConfig.retryIntervalMs + " ms" }

                Text { color: "#aaa"; text: "Resolution:" }
                Text { color: "white"; text: appConfig.targetWidth + "×" + appConfig.targetHeight }

                Text { color: "#aaa"; text: "Playlist Root:" }
                Text { color: "white"; text: appConfig.playlistRoot; elide: Text.ElideMiddle;
                       Layout.maximumWidth: 300 }

                Text { color: "#aaa"; text: "Audio:" }
                Text { color: "white"; text: appConfig.audioEnabled ? "Enabled" : "Disabled" }
            }

            Rectangle { width: parent.width; height: 1; color: "#333" }

            // Playlist status
            Text {
                text: "Playlists"
                font.pixelSize: 16
                font.bold: true
                color: "white"
            }

            GridLayout {
                columns: 2
                columnSpacing: 20
                rowSpacing: 8

                Text { color: "#aaa"; text: "Background:" }
                Text { color: "white"; text: backgroundPlayer.playlistSize + " files" }

                Text { color: "#aaa"; text: "Main:" }
                Text { color: "white"; text: mainPlayer.playlistSize + " files" }

                Text { color: "#aaa"; text: "Horizontal:" }
                Text { color: "white"; text: horizontalPlayer.playlistSize + " files" }

                Text { color: "#aaa"; text: "Vertical:" }
                Text { color: "white"; text: verticalPlayer.playlistSize + " files" }
            }

            // Action buttons
            Row {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter

                Button {
                    text: "Rescan Playlists"
                    onClicked: playlistService.scanAll()
                    palette.buttonText: "white"
                    palette.button: "#0f3460"
                }

                Button {
                    text: "Reload Config"
                    onClicked: appConfig.reload()
                    palette.buttonText: "white"
                    palette.button: "#0f3460"
                }

                Button {
                    text: "Close"
                    onClicked: root.appState = "player"
                    palette.buttonText: "white"
                    palette.button: "#e94560"
                }
            }
        }
    }
}
