#include "services/CliService.h"
#include <QDebug>

CliService::CliService(QObject *parent)
    : QObject(parent)
{
}

void CliService::parse(const QGuiApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("NCTV Digital Signage Player");
    parser.addHelpOption();
    parser.addVersionOption();

    // Define options
    QCommandLineOption kioskOption("kiosk", "Enable kiosk mode (fullscreen, no escape key)");
    QCommandLineOption noOptimizeOption("no-optimize", "Skip video optimization on startup");
    QCommandLineOption debugOption("debug", "Enable verbose debug logging");
    QCommandLineOption playlistOption("playlist", "Override playlist root directory", "directory");
    QCommandLineOption configOption("config", "Override config file path", "file");

    parser.addOption(kioskOption);
    parser.addOption(noOptimizeOption);
    parser.addOption(debugOption);
    parser.addOption(playlistOption);
    parser.addOption(configOption);

    parser.process(app);

    m_kioskMode   = parser.isSet(kioskOption);
    m_noOptimize  = parser.isSet(noOptimizeOption);
    m_debugMode   = parser.isSet(debugOption);
    m_playlistDir = parser.value(playlistOption);
    m_configFile  = parser.value(configOption);

    qInfo() << "[CliService] Parsed arguments:"
            << "kiosk=" << m_kioskMode
            << "noOptimize=" << m_noOptimize
            << "debug=" << m_debugMode
            << "playlist=" << m_playlistDir
            << "config=" << m_configFile;
}

bool    CliService::kioskMode() const   { return m_kioskMode; }
bool    CliService::noOptimize() const  { return m_noOptimize; }
bool    CliService::debugMode() const   { return m_debugMode; }
QString CliService::playlistDir() const { return m_playlistDir; }
QString CliService::configFile() const  { return m_configFile; }
