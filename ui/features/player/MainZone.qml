import QtQuick
import QtQuick.Window

/**
 * MainZone.qml - Primary content zone (top-left area).
 * Z: 1 — renders above the background zone.
 * Background is transparent to allow libVLC hardware overlay underneath.
 */
Item {
    id: mainZoneItem

    Rectangle {
        anchors.fill: parent
        color: "transparent" // Crucial: must be transparent for VLC overlay
    }

    Image {
        id: mainImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        visible: mainPlayer.showImage
        source: mainPlayer.currentImageSource
        cache: false
        asynchronous: true
    }

    Behavior on opacity {
        NumberAnimation { duration: 500; easing.type: Easing.InOutQuad }
    }

    function updateGeometry() {
        if (mainPlayer && windowService) {
            var globalPos = mapToItem(null, 0, 0);
            mainPlayer.setGeometry(globalPos.x, globalPos.y, width, height);
            mainPlayer.setZOrder(z); // Sync native window Z-order with QML Z-order
            console.log("[MainZone] Geometry update: " + globalPos.x + "," + globalPos.y + " " + width + "x" + height);
        }
    }

    onWidthChanged: updateGeometry()
    onHeightChanged: updateGeometry()
    onXChanged: updateGeometry()
    onYChanged: updateGeometry()
    onZChanged: updateGeometry()

    Component.onCompleted: {
        console.log("[MainZone] Initialized (" + width + "x" + height + ")");

        if (mainPlayer && windowService) {
            mainPlayer.setWindowId(windowService.windowId());
            updateGeometry();
        } else {
            console.error("[MainZone] mainPlayer or windowService not available");
        }
    }
}