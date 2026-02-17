#include "player/ZonePlayer.h"
#include "services/WindowService.h"

#include <QFileInfo>
#include <QDebug>
#include <QUrl>
#include <QDir>
#include <QCoreApplication>
#include <QGuiApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// ──────────────────────────────────────────────
// Supported Extensions
// ──────────────────────────────────────────────
const QStringList ZonePlayer::s_imageExtensions = {
    "jpg", "jpeg", "png", "bmp", "gif", "webp", "svg"
};

const QStringList ZonePlayer::s_videoExtensions = {
    "mp4", "mkv", "avi", "mov", "wmv", "flv", "webm", "ts", "m4v", "mpg", "mpeg"
};

// ──────────────────────────────────────────────
// Constructor / Destructor
// ──────────────────────────────────────────────
ZonePlayer::ZonePlayer(const QString &zoneName, QObject *parent)
    : QObject(parent)
    , m_zoneName(zoneName)
{
    // Image duration timer (single-shot → advances to next item)
    m_imageTimer.setSingleShot(true);
    connect(&m_imageTimer, &QTimer::timeout, this, &ZonePlayer::onImageTimerTimeout);

    initVlc();
    qInfo() << "[ZonePlayer]" << m_zoneName << "created";
}

ZonePlayer::~ZonePlayer()
{
    stop();
    releaseVlc();
    destroyZoneWindow();
    qInfo() << "[ZonePlayer]" << m_zoneName << "destroyed";
}

// ──────────────────────────────────────────────
// libVLC Initialization
// ──────────────────────────────────────────────
void ZonePlayer::initVlc()
{
#ifdef Q_OS_WIN
    // Set VLC plugin path for Windows (relative to executable)
    static bool pluginPathSet = false;
    if (!pluginPathSet) {
        QString pluginPath = QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");
        if (QDir(pluginPath).exists()) {
            qputenv("VLC_PLUGIN_PATH", pluginPath.toUtf8());
            qInfo() << "[ZonePlayer] VLC_PLUGIN_PATH:" << pluginPath;
        }
        pluginPathSet = true;
    }
#endif

#ifdef Q_OS_WIN
    const char *args[] = {
        "--no-xlib",              // No X11 threading issues (harmless on Windows)
        "--no-video-title-show",  // Don't overlay filename on video
        "--quiet",                // Reduce VLC noise
        "--no-audio",             // Digital signage typically muted
    };
#else
    const char *args[] = {
        "--avcodec-hw=v4l2_m2m", // Use Video4Linux2 Memory-to-Memory hardware decoding
        "--no-xlib",             // Skip X11 overhead if possible
    };
#endif

    m_vlcInstance = libvlc_new(sizeof(args) / sizeof(args[0]), args);
    if (!m_vlcInstance) {
        qCritical() << "[ZonePlayer]" << m_zoneName << "FATAL: Failed to create libVLC instance";
        emit errorOccurred("Failed to create libVLC instance");
        return;
    }

    m_vlcPlayer = libvlc_media_player_new(m_vlcInstance);
    if (!m_vlcPlayer) {
        qCritical() << "[ZonePlayer]" << m_zoneName << "FATAL: Failed to create libVLC media player";
        emit errorOccurred("Failed to create libVLC media player");
        return;
    }

    // Register for end-of-media events
    m_vlcEvents = libvlc_media_player_event_manager(m_vlcPlayer);
    if (m_vlcEvents) {
        libvlc_event_attach(m_vlcEvents, libvlc_MediaPlayerEndReached,
                            vlcEventCallback, this);
        libvlc_event_attach(m_vlcEvents, libvlc_MediaPlayerEncounteredError,
                            vlcEventCallback, this);
        libvlc_event_attach(m_vlcEvents, libvlc_MediaPlayerPlaying,
                            vlcEventCallback, this);
    }

    qInfo() << "[ZonePlayer]" << m_zoneName << "libVLC initialized";
}

void ZonePlayer::releaseVlc()
{
    if (m_vlcPlayer) {
        libvlc_media_player_stop(m_vlcPlayer);
        libvlc_media_player_release(m_vlcPlayer);
        m_vlcPlayer = nullptr;
    }
    if (m_vlcInstance) {
        libvlc_release(m_vlcInstance);
        m_vlcInstance = nullptr;
    }
}

// ──────────────────────────────────────────────
// Static VLC Event Callback
// ──────────────────────────────────────────────
void ZonePlayer::vlcEventCallback(const libvlc_event_t *event, void *userData)
{
    auto *self = static_cast<ZonePlayer *>(userData);
    if (!self) return;

    switch (event->type) {
    case libvlc_MediaPlayerEndReached:
        // Use queued invocation so we're on the Qt thread
        QMetaObject::invokeMethod(self, "onMediaEndReached", Qt::QueuedConnection);
        break;
    case libvlc_MediaPlayerEncounteredError:
        qWarning() << "[ZonePlayer]" << self->m_zoneName << "VLC playback error";
        QMetaObject::invokeMethod(self, "onMediaEndReached", Qt::QueuedConnection);
        break;
    case libvlc_MediaPlayerPlaying:
        QMetaObject::invokeMethod(self, "checkVideoResolution", Qt::QueuedConnection);
        break;
    default:
        break;
    }
}

// ──────────────────────────────────────────────
// Accessors
// ──────────────────────────────────────────────
QString ZonePlayer::zoneName() const           { return m_zoneName; }
bool    ZonePlayer::isPlaying() const          { return m_isPlaying; }
bool    ZonePlayer::showImage() const          { return m_showImage; }
QString ZonePlayer::currentImageSource() const { return m_currentImageSrc; }
bool    ZonePlayer::is4K() const               { return m_is4K; }
QString ZonePlayer::currentMediaPath() const   { return m_currentMediaPath; }
int     ZonePlayer::currentIndex() const       { return m_currentIndex; }
int     ZonePlayer::playlistSize() const       { return m_playlist.size(); }

// ──────────────────────────────────────────────
// Configuration
// ──────────────────────────────────────────────
void ZonePlayer::setImageDuration(int ms)
{
    m_imageDurationMs = ms;
    qDebug() << "[ZonePlayer]" << m_zoneName << "Image duration set to" << ms << "ms";
}

void ZonePlayer::setGeometry(int x, int y, int w, int h)
{
    m_geometry = QRect(x, y, w, h);
    qDebug() << "[ZonePlayer]" << m_zoneName << "Geometry set to" << m_geometry;
    createZoneWindow();
}

void ZonePlayer::setWindowId(quintptr winId)
{
    m_windowId = winId;
    qDebug() << "[ZonePlayer]" << m_zoneName << "Parent window ID set to" << winId;
    createZoneWindow();
}

void ZonePlayer::setZOrder(int z)
{
    m_zOrder = z;
    if (m_zoneWindow) {
        if (z <= 0)
            m_zoneWindow->lower();
        else
            m_zoneWindow->raise();
    }
    qDebug() << "[ZonePlayer]" << m_zoneName << "Z-order set to" << z;
}

void ZonePlayer::createZoneWindow()
{
    // Need both parent window ID and valid geometry
    if (!m_windowId || !m_geometry.isValid())
        return;

    // Get parent QWindow from WindowService
    QWindow *parentWindow = nullptr;
    auto *ws = WindowService::instance();
    if (ws)
        parentWindow = ws->mainWindow();
    if (!parentWindow) {
        qWarning() << "[ZonePlayer]" << m_zoneName << "Parent window not available yet";
        return;
    }

    // Optimization: Reuse existing window if parent hasn't changed.
    // This prevents VLC black screen issues caused by destroying the HWND during playback.
    if (m_zoneWindow && m_zoneWindow->parent() == parentWindow) {
        m_zoneWindow->setGeometry(m_geometry);
        if (m_zOrder <= 0)
            m_zoneWindow->lower();
        else
            m_zoneWindow->raise();
        return;
    }

    // Tear down any existing child window
    destroyZoneWindow();

    // Create a native child window positioned at the zone coordinates
    m_zoneWindow = new QWindow();
    m_zoneWindow->setParent(parentWindow);
    m_zoneWindow->setGeometry(m_geometry);
    m_zoneWindow->setFlag(Qt::FramelessWindowHint);
    m_zoneWindow->setObjectName(m_zoneName + QStringLiteral("_vlc"));

    // Force native window handle creation
    m_zoneWindow->create();
    m_zoneWindow->show();

    // Apply z-ordering
    if (m_zOrder <= 0)
        m_zoneWindow->lower();
    else
        m_zoneWindow->raise();

    // Attach libVLC to render into this child window
    quintptr childId = m_zoneWindow->winId();
    if (m_vlcPlayer) {
#ifdef Q_OS_WIN
        libvlc_media_player_set_hwnd(m_vlcPlayer, reinterpret_cast<void *>(childId));
#elif defined(Q_OS_LINUX)
        libvlc_media_player_set_xwindow(m_vlcPlayer, static_cast<uint32_t>(childId));
#endif
    }

    // Start hidden — shown only when video is actively playing
    m_zoneWindow->hide();

    qInfo() << "[ZonePlayer]" << m_zoneName
            << "Zone window created at" << m_geometry
            << "childWinId:" << childId << "z:" << m_zOrder;
}

void ZonePlayer::destroyZoneWindow()
{
    if (m_zoneWindow) {
        m_zoneWindow->hide();
        m_zoneWindow->destroy();
        delete m_zoneWindow;
        m_zoneWindow = nullptr;
    }
}

// ──────────────────────────────────────────────
// Playlist Management
// ──────────────────────────────────────────────
void ZonePlayer::setPlaylist(const QStringList &files)
{
    stop();
    m_playlist = files;
    m_currentIndex = 0;

    emit playlistSizeChanged();
    emit currentIndexChanged();

    qInfo() << "[ZonePlayer]" << m_zoneName
            << "Playlist loaded:" << m_playlist.size() << "items";
}

// ──────────────────────────────────────────────
// Playback Controls
// ──────────────────────────────────────────────
void ZonePlayer::play()
{
    if (m_playlist.isEmpty()) {
        qWarning() << "[ZonePlayer]" << m_zoneName << "Cannot play: playlist is empty";
        return;
    }
    playCurrentItem();
}

void ZonePlayer::stop()
{
    m_imageTimer.stop();

    if (m_vlcPlayer) {
        libvlc_media_player_stop(m_vlcPlayer);
    }

    // Hide the native child window
    if (m_zoneWindow) {
        m_zoneWindow->hide();
    }

    m_isPlaying = false;
    emit isPlayingChanged();

    // Hide image layer
    if (m_showImage) {
        m_showImage = false;
        emit showImageChanged();
    }
}

void ZonePlayer::next()
{
    if (m_playlist.isEmpty()) return;

    m_currentIndex = (m_currentIndex + 1) % m_playlist.size();
    emit currentIndexChanged();

    if (m_isPlaying) {
        playCurrentItem();
    }
}

void ZonePlayer::previous()
{
    if (m_playlist.isEmpty()) return;

    m_currentIndex = (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
    emit currentIndexChanged();

    if (m_isPlaying) {
        playCurrentItem();
    }
}

// ──────────────────────────────────────────────
// Core Playback Logic
// ──────────────────────────────────────────────
void ZonePlayer::playCurrentItem()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
        m_currentIndex = 0;
    }

    const QString &filePath = m_playlist.at(m_currentIndex);
    m_currentMediaPath = filePath;
    emit currentMediaPathChanged();

    qInfo() << "[ZonePlayer]" << m_zoneName
            << "Playing [" << (m_currentIndex + 1) << "/" << m_playlist.size() << "]:"
            << filePath;

    if (isImageFile(filePath)) {
        showStaticImage(filePath);
    } else if (isVideoFile(filePath)) {
        playVideo(filePath);
    } else {
        qWarning() << "[ZonePlayer]" << m_zoneName << "Unsupported file type:" << filePath;
        // Skip to next
        QMetaObject::invokeMethod(this, "onMediaEndReached", Qt::QueuedConnection);
    }
}

void ZonePlayer::playVideo(const QString &filePath)
{
    // Hide QML image layer
    if (m_showImage) {
        m_showImage = false;
        emit showImageChanged();
    }
    m_imageTimer.stop();

    // Reset 4K status (assume not 4K until detected)
    if (m_is4K) {
        m_is4K = false;
        emit is4KChanged();
    }

    if (!m_vlcPlayer || !m_vlcInstance) {
        qCritical() << "[ZonePlayer]" << m_zoneName << "VLC not initialized";
        return;
    }

    // Show the native child window for VLC rendering
    if (m_zoneWindow) {
        m_zoneWindow->show();
        if (m_zOrder <= 0)
            m_zoneWindow->lower();
        else
            m_zoneWindow->raise();
    }

    // Create and load the media
    libvlc_media_t *media = libvlc_media_new_path(m_vlcInstance,
        QDir::toNativeSeparators(filePath).toUtf8().constData());

    if (!media) {
        qCritical() << "[ZonePlayer]" << m_zoneName << "Failed to create VLC media:" << filePath;
        emit errorOccurred("Failed to create VLC media for: " + filePath);
        return;
    }

    // Hardware-accelerated decoding hints
    libvlc_media_add_option(media, ":avcodec-hw=any");
    libvlc_media_add_option(media, ":no-video-title-show");

    libvlc_media_player_set_media(m_vlcPlayer, media);
    libvlc_media_release(media);

    // Let VLC auto-fit within the child window, preserving source aspect ratio.
    // scale=0 means "best fit" (letterbox to preserve aspect ratio).
    // aspect=nullptr means "use source aspect ratio".
    libvlc_video_set_scale(m_vlcPlayer, 0);
    libvlc_video_set_aspect_ratio(m_vlcPlayer, nullptr);

    if (libvlc_media_player_play(m_vlcPlayer) == 0) {
        m_isPlaying = true;
        emit isPlayingChanged();
    } else {
        qCritical() << "[ZonePlayer]" << m_zoneName << "VLC play() failed for:" << filePath;
        emit errorOccurred("VLC play() failed for: " + filePath);
    }
}

void ZonePlayer::showStaticImage(const QString &filePath)
{
    // Stop any VLC video playback and hide the native zone window
    if (m_vlcPlayer) {
    if (m_is4K) {
        m_is4K = false;
        emit is4KChanged();
    }

        libvlc_media_player_stop(m_vlcPlayer);
    }
    if (m_zoneWindow) {
        m_zoneWindow->hide();
    }

    // Set image source for QML Image component
    m_currentImageSrc = QUrl::fromLocalFile(filePath).toString();
    emit currentImageSourceChanged();

    // Show the QML image layer
    m_showImage = true;
    emit showImageChanged();

    m_isPlaying = true;
    emit isPlayingChanged();

    // Start timer to advance after imageDurationMs
    m_imageTimer.start(m_imageDurationMs);

    qDebug() << "[ZonePlayer]" << m_zoneName
             << "Showing image for" << m_imageDurationMs << "ms:" << filePath;
}

// ──────────────────────────────────────────────
// Timer / Event Handlers
// ──────────────────────────────────────────────
void ZonePlayer::onImageTimerTimeout()
{
    qDebug() << "[ZonePlayer]" << m_zoneName << "Image timer expired, advancing...";
    emit mediaFinished();
    next();
}

void ZonePlayer::onMediaEndReached()
{
    qDebug() << "[ZonePlayer]" << m_zoneName << "Media end reached, advancing...";
    emit mediaFinished();

    // Advance to next item (loops via modulo in next())
    next();
}

void ZonePlayer::checkVideoResolution()
{
    if (!m_vlcPlayer) return;

    unsigned width = 0, height = 0;
    if (libvlc_video_get_size(m_vlcPlayer, 0, &width, &height) == 0) {
        // Typically 4K is 3840x2160 or 4096x2160.
        // We'll treat anything >= 3800 width as "4K" for this purpose.
        bool newIs4K = (width >= 3800);
        if (m_is4K != newIs4K) {
            m_is4K = newIs4K;
            emit is4KChanged();
            qInfo() << "[ZonePlayer]" << m_zoneName
                    << "Video resolution detected:" << width << "x" << height
                    << "is4K:" << m_is4K;
        }
    }
}

// ──────────────────────────────────────────────
// File Type Detection
// ──────────────────────────────────────────────
bool ZonePlayer::isImageFile(const QString &filePath) const
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    return s_imageExtensions.contains(ext);
}

bool ZonePlayer::isVideoFile(const QString &filePath) const
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    return s_videoExtensions.contains(ext);
}
