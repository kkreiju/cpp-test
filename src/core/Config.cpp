#include "core/Config.h"

#include <QSettings>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

// ──────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────
Config::Config(QObject *parent)
    : QObject(parent)
{
    // Set platform-appropriate defaults
#ifdef NCTV_PLATFORM_PI
    m_playlistRoot = QStringLiteral("/var/lib/nctv-player/playlist");
    m_logPath      = QStringLiteral("/var/log/nctv-player.log");
#else
    // Desktop development defaults
    m_playlistRoot = QCoreApplication::applicationDirPath() + QStringLiteral("/../playlist");
    m_logPath      = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                     + QStringLiteral("/nctv-player.log");
#endif
}

// ──────────────────────────────────────────────
// Config File Resolution
// ──────────────────────────────────────────────
QString Config::resolveConfigPath() const
{
    // Priority order:
    // 1. /etc/nctv-player/config.ini  (Pi / production)
    // 2. ./config.ini                 (local development)
    // 3. AppData fallback             (Windows)

    const QString etcPath = QStringLiteral("/etc/nctv-player/config.ini");
    if (QFileInfo::exists(etcPath))
        return etcPath;

    const QString localPath = QCoreApplication::applicationDirPath() + QStringLiteral("/config.ini");
    if (QFileInfo::exists(localPath))
        return localPath;

    // Check next to the source tree (for dev builds)
    const QString srcPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../config.ini");
    if (QFileInfo::exists(srcPath))
        return srcPath;

    // Return the /etc path even if it doesn't exist — we'll use defaults
    qWarning() << "[Config] No config file found, using defaults";
    return etcPath;
}

// ──────────────────────────────────────────────
// Load / Reload
// ──────────────────────────────────────────────
void Config::load()
{
    const QString path = resolveConfigPath();
    qInfo() << "[Config] Loading configuration from:" << path;

    if (!QFileInfo::exists(path)) {
        qWarning() << "[Config] Config file not found, using defaults";
        emit configChanged();
        return;
    }

    QSettings settings(path, QSettings::IniFormat);

    // [General]
    settings.beginGroup(QStringLiteral("General"));
    m_kioskMode       = settings.value("kioskMode", m_kioskMode).toBool();
    m_retryIntervalMs = settings.value("retryIntervalMs", m_retryIntervalMs).toInt();
    m_imageDurationMs = settings.value("imageDurationMs", m_imageDurationMs).toInt();
    m_audioEnabled    = settings.value("audioEnabled", m_audioEnabled).toBool();
    settings.endGroup();

    // [Paths]
    settings.beginGroup(QStringLiteral("Paths"));
    m_playlistRoot = settings.value("playlistRoot", m_playlistRoot).toString();
    m_logPath      = settings.value("logPath", m_logPath).toString();
    settings.endGroup();

    // [Display]
    settings.beginGroup(QStringLiteral("Display"));
    m_targetWidth  = settings.value("targetWidth", m_targetWidth).toInt();
    m_targetHeight = settings.value("targetHeight", m_targetHeight).toInt();
    settings.endGroup();

    // [Optimization]
    settings.beginGroup(QStringLiteral("Optimization"));
    m_optimizedSuffix = settings.value("optimizedSuffix", m_optimizedSuffix).toString();
    settings.endGroup();

    qInfo() << "[Config] Loaded:"
            << "kiosk=" << m_kioskMode
            << "retry=" << m_retryIntervalMs << "ms"
            << "imageDur=" << m_imageDurationMs << "ms"
            << "playlist=" << m_playlistRoot
            << "resolution=" << m_targetWidth << "x" << m_targetHeight;

    emit configChanged();
}

void Config::reload()
{
    qInfo() << "[Config] Reloading configuration...";
    load();
}

// ──────────────────────────────────────────────
// Accessors
// ──────────────────────────────────────────────
bool    Config::kioskMode() const       { return m_kioskMode; }
int     Config::retryIntervalMs() const { return m_retryIntervalMs; }
int     Config::imageDurationMs() const { return m_imageDurationMs; }
QString Config::playlistRoot() const    { return m_playlistRoot; }
QString Config::logPath() const         { return m_logPath; }
int     Config::targetWidth() const     { return m_targetWidth; }
int     Config::targetHeight() const    { return m_targetHeight; }
bool    Config::audioEnabled() const    { return m_audioEnabled; }
QString Config::optimizedSuffix() const { return m_optimizedSuffix; }

QVariantMap Config::toMap() const
{
    return {
        {"kioskMode",       m_kioskMode},
        {"retryIntervalMs", m_retryIntervalMs},
        {"imageDurationMs", m_imageDurationMs},
        {"playlistRoot",    m_playlistRoot},
        {"logPath",         m_logPath},
        {"targetWidth",     m_targetWidth},
        {"targetHeight",    m_targetHeight},
        {"audioEnabled",    m_audioEnabled},
        {"optimizedSuffix", m_optimizedSuffix},
    };
}
