#ifndef CLISERVICE_H
#define CLISERVICE_H

#include <QObject>
#include <QCommandLineParser>
#include <QGuiApplication>

/**
 * CliService - Command-line argument parser.
 *
 * Supported flags:
 *   --kiosk         Enable kiosk mode (fullscreen, no escape)
 *   --playlist <dir> Override playlist root directory
 *   --config <file>  Override config file path
 *   --no-optimize    Skip video optimization on startup
 *   --debug          Enable verbose debug logging
 */
class CliService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool   kioskMode    READ kioskMode    CONSTANT)
    Q_PROPERTY(bool   noOptimize   READ noOptimize   CONSTANT)
    Q_PROPERTY(bool   debugMode    READ debugMode    CONSTANT)
    Q_PROPERTY(QString playlistDir READ playlistDir  CONSTANT)
    Q_PROPERTY(QString configFile  READ configFile   CONSTANT)

public:
    explicit CliService(QObject *parent = nullptr);
    ~CliService() override = default;

    void parse(const QGuiApplication &app);

    bool    kioskMode() const;
    bool    noOptimize() const;
    bool    debugMode() const;
    QString playlistDir() const;
    QString configFile() const;

private:
    bool    m_kioskMode   = false;
    bool    m_noOptimize  = false;
    bool    m_debugMode   = false;
    QString m_playlistDir;
    QString m_configFile;
};

#endif // CLISERVICE_H
