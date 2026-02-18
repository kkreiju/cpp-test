import QtQuick

/**
 * HorizontalZone.qml - Bottom banner / ticker zone.
 * Z: 1 — renders above the background zone.
 * Background is transparent to allow libVLC hardware overlay underneath.
 */
Item {
    id: hZone

    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    Image {
        id: hImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        visible: horizontalPlayer.showImage
        source: horizontalPlayer.currentImageSource
        cache: false
        asynchronous: true
    }

    Behavior on opacity {
        NumberAnimation { duration: 500; easing.type: Easing.InOutQuad }
    }

    function updateGeometry() {
        if (horizontalPlayer && windowService) {
            var globalPos = mapToItem(null, 0, 0);
            horizontalPlayer.setGeometry(globalPos.x, globalPos.y, width, height);
            horizontalPlayer.setZOrder(z);
            console.log("[HorizontalZone] Geometry update: " + globalPos.x + "," + globalPos.y + " " + width + "x" + height);
        }
    }

    onWidthChanged: updateGeometry()
    onHeightChanged: updateGeometry()
    onXChanged: updateGeometry()
    onYChanged: updateGeometry()

    Component.onCompleted: {
        console.log("[HorizontalZone] Initialized (" + width + "x" + height + ")");
        if (horizontalPlayer && windowService) {
            horizontalPlayer.setWindowId(windowService.windowId());
            updateGeometry();
        }
    }
}
