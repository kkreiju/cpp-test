#include "services/PlaylistService.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

// ──────────────────────────────────────────────
// Supported Extensions
// ──────────────────────────────────────────────
const QStringList PlaylistService::s_supportedExtensions = {
    // Video
    "mp4", "mkv", "avi", "mov", "wmv", "flv", "webm", "ts", "m4v", "mpg", "mpeg",
    // Image
    "jpg", "jpeg", "png", "bmp", "gif", "webp", "svg"
};

// ──────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────
PlaylistService::PlaylistService(QObject *parent)
    : QObject(parent)
{
}

// ──────────────────────────────────────────────
// Configuration
// ──────────────────────────────────────────────
void PlaylistService::setPlaylistRoot(const QString &root)
{
    m_playlistRoot = root;
    qInfo() << "[PlaylistService] Playlist root set to:" << m_playlistRoot;
}

void PlaylistService::setOptimizedSuffix(const QString &suffix)
{
    m_optimizedSuffix = suffix;
}

// ──────────────────────────────────────────────
// Full Scan
// ──────────────────────────────────────────────
void PlaylistService::scanAll()
{
    m_isScanning = true;
    emit isScanningChanged();

    qInfo() << "[PlaylistService] Scanning all playlists from:" << m_playlistRoot;

    m_backgroundFiles = resolveOptimizedFiles(
        scanDirectory(m_playlistRoot + QStringLiteral("/playlist-background")));

    m_mainFiles = resolveOptimizedFiles(
        scanDirectory(m_playlistRoot + QStringLiteral("/playlist-main")));

    m_horizontalFiles = resolveOptimizedFiles(
        scanDirectory(m_playlistRoot + QStringLiteral("/playlist-horizontal")));

    m_verticalFiles = resolveOptimizedFiles(
        scanDirectory(m_playlistRoot + QStringLiteral("/playlist-vertical")));

    const int total = totalFileCount();

    qInfo() << "[PlaylistService] Scan complete. Total files:" << total
            << "| BG:" << m_backgroundFiles.size()
            << "| Main:" << m_mainFiles.size()
            << "| Horiz:" << m_horizontalFiles.size()
            << "| Vert:" << m_verticalFiles.size();

    m_isScanning = false;
    emit isScanningChanged();
    emit playlistsChanged();
    emit scanComplete(total);
}

void PlaylistService::scanZone(const QString &zoneName)
{
    qInfo() << "[PlaylistService] Scanning zone:" << zoneName;

    if (zoneName == "background") {
        m_backgroundFiles = resolveOptimizedFiles(
            scanDirectory(m_playlistRoot + QStringLiteral("/playlist-background")));
    } else if (zoneName == "main") {
        m_mainFiles = resolveOptimizedFiles(
            scanDirectory(m_playlistRoot + QStringLiteral("/playlist-main")));
    } else if (zoneName == "horizontal") {
        m_horizontalFiles = resolveOptimizedFiles(
            scanDirectory(m_playlistRoot + QStringLiteral("/playlist-horizontal")));
    } else if (zoneName == "vertical") {
        m_verticalFiles = resolveOptimizedFiles(
            scanDirectory(m_playlistRoot + QStringLiteral("/playlist-vertical")));
    } else {
        qWarning() << "[PlaylistService] Unknown zone:" << zoneName;
        return;
    }

    emit playlistsChanged();
}

// ──────────────────────────────────────────────
// Accessors
// ──────────────────────────────────────────────
QStringList PlaylistService::backgroundFiles() const  { return m_backgroundFiles; }
QStringList PlaylistService::mainFiles() const        { return m_mainFiles; }
QStringList PlaylistService::horizontalFiles() const  { return m_horizontalFiles; }
QStringList PlaylistService::verticalFiles() const    { return m_verticalFiles; }
bool        PlaylistService::isScanning() const       { return m_isScanning; }

QStringList PlaylistService::filesForZone(const QString &zoneName) const
{
    if (zoneName == "background")  return m_backgroundFiles;
    if (zoneName == "main")        return m_mainFiles;
    if (zoneName == "horizontal")  return m_horizontalFiles;
    if (zoneName == "vertical")    return m_verticalFiles;
    return {};
}

int PlaylistService::totalFileCount() const
{
    return m_backgroundFiles.size()
         + m_mainFiles.size()
         + m_horizontalFiles.size()
         + m_verticalFiles.size();
}

// ──────────────────────────────────────────────
// Directory Scanning
// ──────────────────────────────────────────────
QStringList PlaylistService::scanDirectory(const QString &dirPath) const
{
    QStringList result;

    QDir dir(dirPath);
    if (!dir.exists()) {
        qWarning() << "[PlaylistService] Directory does not exist:" << dirPath;
        return result;
    }

    QDirIterator it(dirPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        if (isSupportedExtension(fi.suffix().toLower())) {
            result.append(fi.absoluteFilePath());
        }
    }

    // Sort alphabetically for deterministic playlist order
    std::sort(result.begin(), result.end());

    qDebug() << "[PlaylistService] Scanned" << dirPath << "→" << result.size() << "files";
    return result;
}

bool PlaylistService::isSupportedExtension(const QString &ext) const
{
    return s_supportedExtensions.contains(ext);
}

// ──────────────────────────────────────────────
// Optimized File Resolution
// ──────────────────────────────────────────────
// For each file, if an optimized version exists (e.g., video_optimized.mp4),
// prefer it over the raw version. Skip raw files that have optimized twins.
QStringList PlaylistService::resolveOptimizedFiles(const QStringList &rawFiles) const
{
    QMap<QString, QString> bestFiles; // baseName → absolutePath

    for (const QString &filePath : rawFiles) {
        QFileInfo fi(filePath);
        QString baseName = fi.completeBaseName();
        const QString ext = fi.suffix();
        const QString dir = fi.absolutePath();

        // Check if this IS an optimized file
        const bool isOptimized = baseName.endsWith(m_optimizedSuffix);

        if (isOptimized) {
            // Remove the suffix to get the original base name
            const QString originalBase = baseName.left(baseName.length() - m_optimizedSuffix.length());
            const QString key = dir + "/" + originalBase;
            // Optimized always wins
            bestFiles[key] = filePath;
        } else {
            const QString key = dir + "/" + baseName;
            // Only insert raw if no optimized version already found
            if (!bestFiles.contains(key)) {
                // Check if an optimized version exists on disk
                const QString optimizedPath = dir + "/" + baseName + m_optimizedSuffix + "." + ext;
                if (QFileInfo::exists(optimizedPath)) {
                    bestFiles[key] = optimizedPath;
                } else {
                    bestFiles[key] = filePath;
                }
            }
        }
    }

    QStringList result = bestFiles.values();
    std::sort(result.begin(), result.end());
    return result;
}
