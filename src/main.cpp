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

// ──────────────────────────────────────────────
// File-Based Rotating Logger
// ──────────────────────────────────────────────
// ──────────────────────────────────────────────
// Systemd / Journalctl Logging Integration
// ──────────────────────────────────────────────
static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Level prefix
    const char *level = nullptr;
    switch (type) {
        case QtDebugMsg:    level = "DEBUG"; break;
        case QtInfoMsg:     level = "INFO "; break;
        case QtWarningMsg:  level = "WARN "; break;
        case QtCriticalMsg: level = "CRIT "; break;
        case QtFatalMsg:    level = "FATAL"; break;
    }

    // Format: [Timestamp] [Level] Message (File:Line)
    // We use fprintf(stderr) for direct unbuffered output to systemd/journalctl
    QString formattedMsg = QStringLiteral("[%1] [%2] %3 (%4:%5)\n")
                             .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
                                  level,
                                  msg,
                                  context.file ? context.file : "unknown",
                                  QString::number(context.line));

    fprintf(stderr, "%s", formattedMsg.toLocal8Bit().constData());
    
    // Critical: Flush immediately to ensure no logs are lost on crash
    fflush(stderr); 
}

static void initializeLogging()
{
    // Disable buffering for stderr to ensure immediate log output
    setbuf(stderr, nullptr);
    
    qInstallMessageHandler(messageHandler);
    qInfo() << "=== NCTV Player started (Systemd Logging Enabled) ===";
    qInfo() << "Build Version: " << "1.0.2"; 
}

// ──────────────────────────────────────────────
// Application Entry Point
// ──────────────────────────────────────────────
int main(int argc, char *argv[])
{
#ifdef NCTV_PLATFORM_PI
    // Force X11 backend for Raspberry Pi to ensure VLC embedding works
    // This fixes "could not connect to display" when running from systemd/SSH
    if (qEnvironmentVariableIsEmpty("DISPLAY")) {
        qputenv("DISPLAY", ":0");
        fprintf(stderr, "WARNING: DISPLAY not set, defaulting to :0\n");
    }

    // Force Qt to use XCB (X11) backend instead of Wayland/EGLFS
    // We need X11 window IDs for libVLC embedding
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

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

    rootContext->setContextProperty("cliService",        &cliService);
    rootContext->setContextProperty("windowService",     WindowService::instance());

    // Get primary screen resolution
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    QRect screenGeometry = primaryScreen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();
    qInfo() << "Primary screen resolution detected:" << screenWidth << "x" << screenHeight;

    rootContext->setContextProperty("screenWidth", screenWidth);
    rootContext->setContextProperty("screenHeight", screenHeight);

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
    /*
    // Video Optimizer Removed
    QObject::connect(&videoOptimizer, &VideoOptimizer::optimizationFinished,
                     [&playlistService]() {
        qInfo() << "Video optimization complete. Re-scanning playlists...";
        playlistService.scanAll();
    });
    videoOptimizer.startOptimization();
    */

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
