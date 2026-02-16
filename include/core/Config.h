#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QString>
#include <QVariantMap>

/**
 * Config - Application configuration manager.
 *
 * Uses QSettings to read an .ini config file from the filesystem.
 * On Pi:      /etc/nctv-player/config.ini
 * On Desktop: ./config.ini (or AppData location)
 *
 * Exposes all settings as Q_PROPERTY for direct QML binding.
 */
class Config : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool    kioskMode       READ kioskMode       NOTIFY configChanged)
    Q_PROPERTY(int     retryIntervalMs READ retryIntervalMs NOTIFY configChanged)
    Q_PROPERTY(int     imageDurationMs READ imageDurationMs  NOTIFY configChanged)
    Q_PROPERTY(QString playlistRoot    READ playlistRoot     NOTIFY configChanged)
    Q_PROPERTY(QString logPath         READ logPath          NOTIFY configChanged)
    Q_PROPERTY(int     targetWidth     READ targetWidth      NOTIFY configChanged)
    Q_PROPERTY(int     targetHeight    READ targetHeight     NOTIFY configChanged)
    Q_PROPERTY(bool    audioEnabled    READ audioEnabled     NOTIFY configChanged)
    Q_PROPERTY(QString optimizedSuffix READ optimizedSuffix  NOTIFY configChanged)

public:
    explicit Config(QObject *parent = nullptr);
    ~Config() override = default;

    // Load config from filesystem
    Q_INVOKABLE void load();
    Q_INVOKABLE void reload();

    // ── Accessors ──
    bool    kioskMode() const;
    int     retryIntervalMs() const;
    int     imageDurationMs() const;
    QString playlistRoot() const;
    QString logPath() const;
    int     targetWidth() const;
    int     targetHeight() const;
    bool    audioEnabled() const;
    QString optimizedSuffix() const;

    // Full config as a variant map (for QML debugging)
    Q_INVOKABLE QVariantMap toMap() const;

signals:
    void configChanged();

private:
    QString resolveConfigPath() const;

    bool    m_kioskMode       = true;
    int     m_retryIntervalMs = 5000;
    int     m_imageDurationMs = 10000;
    QString m_playlistRoot;
    QString m_logPath;
    int     m_targetWidth     = 1920;
    int     m_targetHeight    = 1080;
    bool    m_audioEnabled    = false;
    QString m_optimizedSuffix = "_optimized";
};

#endif // CONFIG_H
