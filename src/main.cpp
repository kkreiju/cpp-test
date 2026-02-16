/**
 * NCTV Player - Native C++/Qt Digital Signage Application
 * Entry Point: Initializes Qt engine, logging, and loads the QML UI.
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QStandardPaths>
#include <QMutex>
#include <QMutexLocker>

#include "core/Config.h"
#include "services/PlaylistService.h"
#include "services/CliService.h"
#include "services/PidService.h"
#include "services/WindowService.h"
#include "player/ZonePlayer.h"
#include "utils/VideoOptimizer.h"

// ──────────────────────────────────────────────
// File-Based Rotating Logger
// ──────────────────────────────────────────────
static QFile   g_logFile;
static QMutex  g_logMutex;
static constexpr qint64 MAX_LOG_SIZE = 10 * 1024 * 1024; // 10 MB rotation threshold

static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker locker(&g_logMutex);

    // Rotate log if it exceeds maximum size
    if (g_logFile.size() > MAX_LOG_SIZE) {
        g_logFile.close();

        const QString logPath = g_logFile.fileName();
        const QString backupPath = logPath + ".old";

        QFile::remove(backupPath);
        QFile::rename(logPath, backupPath);

        g_logFile.setFileName(logPath);
        g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }

    if (!g_logFile.isOpen())
        return;

    // Level prefix
    const char *level = nullptr;
    switch (type) {
        case QtDebugMsg:    level = "DEBUG"; break;
        case QtInfoMsg:     level = "INFO "; break;
        case QtWarningMsg:  level = "WARN "; break;
        case QtCriticalMsg: level = "CRIT "; break;
        case QtFatalMsg:    level = "FATAL"; break;
    }

    const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    const QString line = QStringLiteral("[%1] [%2] %3 (%4:%5)\n")
                             .arg(timestamp, level, msg,
                                  context.file ? context.file : "unknown",
                                  QString::number(context.line));

    QTextStream stream(&g_logFile);
    stream << line;
    stream.flush();

    // Also mirror to stderr for development
#ifdef NCTV_PLATFORM_DESKTOP
    fprintf(stderr, "%s", line.toLocal8Bit().constData());
#endif
}

static void initializeLogging()
{
    QString logDir;
    QString logPath;

#ifdef NCTV_PLATFORM_PI
    logDir  = QStringLiteral("/var/log");
    logPath = QStringLiteral("/var/log/nctv-player.log");
#else
    logDir  = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    logPath = logDir + QStringLiteral("/nctv-player.log");
#endif

    QDir().mkpath(logDir);

    g_logFile.setFileName(logPath);
    if (g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qInstallMessageHandler(messageHandler);
        qInfo() << "=== NCTV Player started ===" << QDateTime::currentDateTime().toString(Qt::ISODate);
        qInfo() << "Log file:" << logPath;
    } else {
        fprintf(stderr, "WARNING: Could not open log file %s\n", logPath.toLocal8Bit().constData());
    }
}

// ──────────────────────────────────────────────
// Application Entry Point
// ──────────────────────────────────────────────
int main(int argc, char *argv[])
{
    // Initialize logging before anything else
    initializeLogging();

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("nctv-player"));
    app.setOrganizationName(QStringLiteral("NCompass"));
    app.setApplicationVersion(QStringLiteral("1.0.2"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Parse command-line arguments
    CliService cliService;
    cliService.parse(app);

    // PID file guard (single-instance enforcement)
    PidService pidService;
    if (!pidService.acquire()) {
        qCritical() << "Another instance of nctv-player is already running. Exiting.";
        return 1;
    }

    // Load configuration
    Config config;
    config.load();
    qInfo() << "Configuration loaded. Kiosk mode:" << config.kioskMode()
            << "| Image duration:" << config.imageDurationMs() << "ms";

    // Initialize playlist service
    PlaylistService playlistService;
    playlistService.setPlaylistRoot(config.playlistRoot());
    playlistService.scanAll();

    // Initialize zone players (one per zone)
    ZonePlayer backgroundPlayer("background");
    ZonePlayer mainPlayer("main");
    ZonePlayer horizontalPlayer("horizontal");
    ZonePlayer verticalPlayer("vertical");

    // Initialize video optimizer
    VideoOptimizer videoOptimizer;
    videoOptimizer.setPlaylistRoot(config.playlistRoot());

    // ──────────────────────────────────────────────
    // QML Engine Setup & C++ → QML Bridge
    // ──────────────────────────────────────────────
    QQmlApplicationEngine engine;

    // Expose C++ objects to QML
    QQmlContext *rootContext = engine.rootContext();
    rootContext->setContextProperty("appConfig",         &config);
    rootContext->setContextProperty("playlistService",   &playlistService);
    rootContext->setContextProperty("backgroundPlayer",  &backgroundPlayer);
    rootContext->setContextProperty("mainPlayer",        &mainPlayer);
    rootContext->setContextProperty("horizontalPlayer",  &horizontalPlayer);
    rootContext->setContextProperty("verticalPlayer",    &verticalPlayer);
    rootContext->setContextProperty("videoOptimizer",    &videoOptimizer);
    rootContext->setContextProperty("cliService",        &cliService);
    rootContext->setContextProperty("windowService",     WindowService::instance());

    // Load root QML
    const QUrl mainQml(QStringLiteral("qrc:/ui/Main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    engine.load(mainQml);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load Main.qml - no root objects created";
        return -1;
    }

    qInfo() << "NCTV Player UI loaded successfully.";

    // Start video optimization in background after UI is up
    QObject::connect(&videoOptimizer, &VideoOptimizer::optimizationFinished,
                     [&playlistService]() {
        qInfo() << "Video optimization complete. Re-scanning playlists...";
        playlistService.scanAll();
    });
    videoOptimizer.startOptimization();

    // Cleanup on exit
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&]() {
        qInfo() << "=== NCTV Player shutting down ===";
        backgroundPlayer.stop();
        mainPlayer.stop();
        horizontalPlayer.stop();
        verticalPlayer.stop();
        pidService.release();
    });

    return app.exec();
}
