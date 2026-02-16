import QtQuick

/**
 * VerticalZone.qml - Right sidebar content zone.
 * Z: 1 — renders above the background zone.
 * Background is transparent to allow libVLC hardware overlay underneath.
 */
Item {
    id: vZone

    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    Image {
        id: vImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        visible: verticalPlayer.showImage
        source: verticalPlayer.currentImageSource
        cache: false
        asynchronous: true
    }

    Behavior on opacity {
        NumberAnimation { duration: 500; easing.type: Easing.InOutQuad }
    }

    Component.onCompleted: {
        console.log("[VerticalZone] Initialized (" + width + "x" + height + ")");
        if (verticalPlayer && windowService) {
            var globalPos = mapToItem(null, 0, 0);
            verticalPlayer.setWindowId(windowService.windowId());
            verticalPlayer.setGeometry(globalPos.x, globalPos.y, width, height);
            verticalPlayer.setZOrder(1);
            console.log("[VerticalZone] Zone window: " + globalPos.x + "," + globalPos.y + " " + width + "x" + height);
        }
    }
}
