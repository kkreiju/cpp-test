#ifndef ZONEPLAYER_H
#define ZONEPLAYER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QRect>
#include <QWindow>
#include <vlc/vlc.h>

/**
 * ZonePlayer - C++ wrapper around libVLC for a single display zone.
 *
 * Handles both video and image playback:
 *  - For videos: libVLC renders hardware-accelerated frames directly
 *    into the zone's screen coordinates.
 *  - For images: Hides the VLC layer and exposes QML-native Image source
 *    via Q_PROPERTY, with a configurable display duration timer.
 *
 * Each zone (background, main, horizontal, vertical) gets its own
 * ZonePlayer instance.
 */
class ZonePlayer : public QObject
{
    Q_OBJECT

    // Properties exposed to QML
    Q_PROPERTY(QString zoneName          READ zoneName          CONSTANT)
    Q_PROPERTY(bool    isPlaying         READ isPlaying         NOTIFY isPlayingChanged)
    Q_PROPERTY(bool    showImage         READ showImage         NOTIFY showImageChanged)
    Q_PROPERTY(QString currentImageSource READ currentImageSource NOTIFY currentImageSourceChanged)
    Q_PROPERTY(QString currentMediaPath  READ currentMediaPath  NOTIFY currentMediaPathChanged)
    Q_PROPERTY(int     currentIndex      READ currentIndex      NOTIFY currentIndexChanged)
    Q_PROPERTY(int     playlistSize      READ playlistSize      NOTIFY playlistSizeChanged)
    Q_PROPERTY(bool    is4K              READ is4K              NOTIFY is4KChanged)

public:
    explicit ZonePlayer(const QString &zoneName, QObject *parent = nullptr);
    ~ZonePlayer();

    // ── Accessors ──
    QString zoneName() const;
    bool    isPlaying() const;
    bool    showImage() const;
    QString currentImageSource() const;
    QString currentMediaPath() const;
    int     currentIndex() const;
    int     playlistSize() const;
    bool    is4K() const;

    // ── Configuration ──
    Q_INVOKABLE void setImageDuration(int ms);
    Q_INVOKABLE void setGeometry(int x, int y, int w, int h);
    Q_INVOKABLE void setWindowId(quintptr winId);
    Q_INVOKABLE void setZOrder(int z);

    // ── Playlist ──
    Q_INVOKABLE void setPlaylist(const QStringList &files);
    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();

signals:
    void isPlayingChanged();
    void showImageChanged();
    void currentImageSourceChanged();
    void currentMediaPathChanged();
    void currentIndexChanged();
    void playlistSizeChanged();
    void is4KChanged();
    void mediaFinished();
    void errorOccurred(const QString &message);

private slots:
    void onImageTimerTimeout();
    void onMediaEndReached();
    void checkVideoResolution();

private:
    // ── Internal helpers ──
    void initVlc();
    void releaseVlc();
    void playCurrentItem();
    void playVideo(const QString &filePath);
    void showStaticImage(const QString &filePath);
    bool isImageFile(const QString &filePath) const;
    bool isVideoFile(const QString &filePath) const;
    void getVideoDimensions(libvlc_media_t *media, unsigned &width, unsigned &height);

    // Per-zone native child window management
    void createZoneWindow();
    void destroyZoneWindow();

    // libVLC event callback (static, forwarded to instance)
    static void vlcEventCallback(const libvlc_event_t *event, void *userData);

    // ── Members ──
    QString         m_zoneName;
    bool            m_isPlaying       = false;
    bool            m_showImage       = false;
    bool            m_is4K            = false;
    QString         m_currentImageSrc;
    QString         m_currentMediaPath;

    QStringList     m_playlist;
    int             m_currentIndex    = 0;

    int             m_imageDurationMs = 10000;  // Default 10 seconds per image
    QTimer          m_imageTimer;

    // Zone screen geometry for libVLC overlay
    QRect           m_geometry;
    quintptr        m_windowId        = 0;
    int             m_zOrder          = 1;

    // Per-zone native child window for libVLC rendering
    QWindow        *m_zoneWindow      = nullptr;

    // libVLC handles
    libvlc_instance_t     *m_vlcInstance = nullptr;
    libvlc_media_player_t *m_vlcPlayer   = nullptr;
    libvlc_event_manager_t *m_vlcEvents  = nullptr;

    // Supported extensions
    static const QStringList s_imageExtensions;
    static const QStringList s_videoExtensions;
};

#endif // ZONEPLAYER_H
