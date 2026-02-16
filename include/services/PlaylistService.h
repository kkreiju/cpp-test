#ifndef PLAYLISTSERVICE_H
#define PLAYLISTSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

/**
 * PlaylistService - Scans the local filesystem for media files per zone.
 *
 * Expected directory layout under playlistRoot:
 *   playlist-background/   → Background zone media
 *   playlist-main/         → Main zone media
 *   playlist-horizontal/   → Horizontal zone media
 *   playlist-vertical/     → Vertical zone media
 *
 * Each folder can contain both "raw" and "optimized" (HEVC) media.
 * When an optimized version exists, it is preferred over the raw file.
 */
class PlaylistService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList backgroundFiles  READ backgroundFiles  NOTIFY playlistsChanged)
    Q_PROPERTY(QStringList mainFiles        READ mainFiles        NOTIFY playlistsChanged)
    Q_PROPERTY(QStringList horizontalFiles  READ horizontalFiles  NOTIFY playlistsChanged)
    Q_PROPERTY(QStringList verticalFiles    READ verticalFiles    NOTIFY playlistsChanged)
    Q_PROPERTY(bool        isScanning       READ isScanning       NOTIFY isScanningChanged)

public:
    explicit PlaylistService(QObject *parent = nullptr);
    ~PlaylistService() override = default;

    // ── Configuration ──
    void setPlaylistRoot(const QString &root);
    void setOptimizedSuffix(const QString &suffix);

    // ── Scanning ──
    Q_INVOKABLE void scanAll();
    Q_INVOKABLE void scanZone(const QString &zoneName);

    // ── Accessors ──
    QStringList backgroundFiles() const;
    QStringList mainFiles() const;
    QStringList horizontalFiles() const;
    QStringList verticalFiles() const;
    bool isScanning() const;

    Q_INVOKABLE QStringList filesForZone(const QString &zoneName) const;
    Q_INVOKABLE int totalFileCount() const;

signals:
    void playlistsChanged();
    void isScanningChanged();
    void scanComplete(int totalFiles);

private:
    QStringList scanDirectory(const QString &dirPath) const;
    bool isSupportedExtension(const QString &ext) const;
    QStringList resolveOptimizedFiles(const QStringList &rawFiles) const;

    QString m_playlistRoot;
    QString m_optimizedSuffix = "_optimized";
    bool    m_isScanning      = false;

    // Per-zone file lists
    QStringList m_backgroundFiles;
    QStringList m_mainFiles;
    QStringList m_horizontalFiles;
    QStringList m_verticalFiles;

    // Supported media extensions
    static const QStringList s_supportedExtensions;
};

#endif // PLAYLISTSERVICE_H
