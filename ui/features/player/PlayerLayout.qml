import QtQuick
import QtQuick.Controls

/**
 * PlayerLayout.qml - Defines the four signage zones with exact coordinates.
 * Replaces player.component from the Electron build.
 *
 * Target resolution: 1920x1080
 * Zone backgrounds are TRANSPARENT so libVLC hardware video overlays
 * can render beneath the Qt scene graph.
 *
 * Z-ordering:
 *   Background Zone (z: 0) - Full screen behind everything
 *   Main Zone       (z: 1) - Primary content area
 *   Horizontal Zone (z: 1) - Bottom ticker/banner
 *   Vertical Zone   (z: 1) - Right sidebar
 *
 * C++ Bridge:
 *   The following context properties are injected from main.cpp:
 *     - playlistService   (PlaylistService*)
 *     - backgroundPlayer  (ZonePlayer*)
 *     - mainPlayer        (ZonePlayer*)
 *     - horizontalPlayer  (ZonePlayer*)
 *     - verticalPlayer    (ZonePlayer*)
 *     - appConfig         (Config*)
 */
Item {
    id: playerLayout
    anchors.fill: parent

    // ── Background Zone (z: 0) ──
    BackgroundZone {
        id: backgroundZone
        visible: !mainPlayer.is4K
        x: 0;  y: 0
        width: 1920;  height: 1080
        z: 0
    }

    // ── Main Zone (z: 1) ──
    MainZone {
        id: mainZone
        x: 0
        y: mainPlayer.is4K ? 0 : 21
        width: mainPlayer.is4K ? 1920 : 1472
        height: mainPlayer.is4K ? 1080 : 828
        z: mainPlayer.is4K ? 10 : 1
    }

    // ── Horizontal Zone (z: 1) ──
    HorizontalZone {
        id: horizontalZone
        visible: !mainPlayer.is4K
        x: 0;  y: 870
        width: 1920;  height: 189
        z: 1
    }

    // ── Vertical Zone (z: 1) ──
    VerticalZone {
        id: verticalZone
        visible: !mainPlayer.is4K
        x: 1472;  y: 21
        width: 448;  height: 849
        z: 1
    }

    // ── Debug Overlay (toggle with F11) ──
    Rectangle {
        id: debugOverlay
        visible: false
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        width: 300; height: 200
        color: "#CC000000"
        radius: 8
        z: 50

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4

            Text { color: "lime"; font.pixelSize: 12
                text: "── NCTV Debug ──" }
            Text { color: "white"; font.pixelSize: 11
                text: "BG:   " + backgroundPlayer.currentIndex + "/" + backgroundPlayer.playlistSize +
                      "  " + (backgroundPlayer.isPlaying ? "▶" : "⏸") }
            Text { color: "white"; font.pixelSize: 11
                text: "Main: " + mainPlayer.currentIndex + "/" + mainPlayer.playlistSize +
                      "  " + (mainPlayer.isPlaying ? "▶" : "⏸") }
            Text { color: "white"; font.pixelSize: 11
                text: "Horz: " + horizontalPlayer.currentIndex + "/" + horizontalPlayer.playlistSize +
                      "  " + (horizontalPlayer.isPlaying ? "▶" : "⏸") }
            Text { color: "white"; font.pixelSize: 11
                text: "Vert: " + verticalPlayer.currentIndex + "/" + verticalPlayer.playlistSize +
                      "  " + (verticalPlayer.isPlaying ? "▶" : "⏸") }
            Text { color: "#aaa"; font.pixelSize: 10
                text: "ImgDur: " + appConfig.imageDurationMs + "ms" }
        }
    }

    Shortcut {
        sequence: "F11"
        onActivated: debugOverlay.visible = !debugOverlay.visible
    }

    // ── Playlist → ZonePlayer Binding ──
    // When PlaylistService emits playlistsChanged, feed files into each ZonePlayer
    Connections {
        target: playlistService
        function onPlaylistsChanged() {
            console.log("[PlayerLayout] Playlists updated, loading into zone players...");
            backgroundPlayer.setPlaylist(playlistService.backgroundFiles);
            mainPlayer.setPlaylist(playlistService.mainFiles);
            horizontalPlayer.setPlaylist(playlistService.horizontalFiles);
            verticalPlayer.setPlaylist(playlistService.verticalFiles);

            // Start playback on all zones
            backgroundPlayer.play();
            mainPlayer.play();
            horizontalPlayer.play();
            verticalPlayer.play();
        }
    }

    Component.onCompleted: {
        console.log("[PlayerLayout] All zones loaded and positioned.");

        // If playlists are already available (scanned before QML loaded), start now
        if (playlistService.backgroundFiles.length > 0 ||
            playlistService.mainFiles.length > 0 ||
            playlistService.horizontalFiles.length > 0 ||
            playlistService.verticalFiles.length > 0) {
            backgroundPlayer.setPlaylist(playlistService.backgroundFiles);
            mainPlayer.setPlaylist(playlistService.mainFiles);
            horizontalPlayer.setPlaylist(playlistService.horizontalFiles);
            verticalPlayer.setPlaylist(playlistService.verticalFiles);

            backgroundPlayer.play();
            mainPlayer.play();
            horizontalPlayer.play();
            verticalPlayer.play();
        }
    }
}
