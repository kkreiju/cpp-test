import QtQuick

/**
 * BackgroundZone.qml - Full-screen background content zone.
 * Z: 0 — renders behind all other zones.
 * Background is transparent to allow libVLC hardware overlay underneath.
 */
Item {
    id: bgZone

    // Transparent so libVLC video overlay can paint underneath
    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    // Image fallback layer — visible when showing a static image
    Image {
        id: bgImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        visible: backgroundPlayer.showImage
        source: backgroundPlayer.currentImageSource
        cache: false
        asynchronous: true
    }

    // Fade transition for media changes
    Behavior on opacity {
        NumberAnimation { duration: 500; easing.type: Easing.InOutQuad }
    }

    function updateGeometry() {
        if (backgroundPlayer && windowService) {
            var globalPos = mapToItem(null, 0, 0);
            backgroundPlayer.setGeometry(globalPos.x, globalPos.y, width, height);
            backgroundPlayer.setZOrder(z);
            console.log("[BackgroundZone] Geometry update: " + globalPos.x + "," + globalPos.y + " " + width + "x" + height);
        }
    }

    onWidthChanged: updateGeometry()
    onHeightChanged: updateGeometry()
    onXChanged: updateGeometry()
    onYChanged: updateGeometry()
    // onZChanged: updateGeometry() // Z usually static, but can add if needed

    Component.onCompleted: {
        console.log("[BackgroundZone] Initialized (" + width + "x" + height + ")");
        if (backgroundPlayer && windowService) {
            backgroundPlayer.setWindowId(windowService.windowId());
            updateGeometry();
        }
    }
}
