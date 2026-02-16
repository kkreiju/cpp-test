#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QStringList>
#include <QRect>

/**
 * Models.h - Core data structures used throughout the application.
 */

namespace nctv {

// ── Media Types ──
enum class MediaType {
    Unknown,
    Video,
    Image
};

// ── Zone Identifiers ──
enum class ZoneId {
    Background,
    Main,
    Horizontal,
    Vertical
};

inline QString zoneIdToString(ZoneId zone) {
    switch (zone) {
        case ZoneId::Background: return QStringLiteral("background");
        case ZoneId::Main:       return QStringLiteral("main");
        case ZoneId::Horizontal: return QStringLiteral("horizontal");
        case ZoneId::Vertical:   return QStringLiteral("vertical");
    }
    return QStringLiteral("unknown");
}

inline QString zoneIdToFolderName(ZoneId zone) {
    return QStringLiteral("playlist-") + zoneIdToString(zone);
}

// ── Media Item ──
struct MediaItem {
    QString   filePath;
    MediaType type       = MediaType::Unknown;
    bool      optimized  = false;
    qint64    fileSize   = 0;
};

// ── Zone Definition (layout coordinates) ──
struct ZoneDefinition {
    ZoneId  id;
    int     x;
    int     y;
    int     width;
    int     height;
    int     zOrder;
};

// Default zone definitions for 1920x1080
inline QList<ZoneDefinition> defaultZoneDefinitions() {
    return {
        { ZoneId::Background, 0,    0,    1920, 1080, 0 },
        { ZoneId::Main,       0,    21,   1472, 828,  1 },
        { ZoneId::Horizontal, 0,    870,  1920, 189,  1 },
        { ZoneId::Vertical,   1472, 21,   448,  849,  1 },
    };
}

// ── Application State ──
enum class AppState {
    Splash,
    Player,
    Menu
};

} // namespace nctv

#endif // MODELS_H
