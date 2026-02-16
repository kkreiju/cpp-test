#include "utils/VideoOptimizer.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>

// ──────────────────────────────────────────────
// Supported Video Extensions
// ──────────────────────────────────────────────
const QStringList VideoOptimizer::s_videoExtensions = {
    "mp4", "mkv", "avi", "mov", "wmv", "flv", "webm", "ts", "m4v", "mpg", "mpeg"
};

// ──────────────────────────────────────────────
// Constructor / Destructor
// ──────────────────────────────────────────────
VideoOptimizer::VideoOptimizer(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VideoOptimizer::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &VideoOptimizer::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &VideoOptimizer::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &VideoOptimizer::onProcessOutput);
}

VideoOptimizer::~VideoOptimizer()
{
    cancelOptimization();
}

// ──────────────────────────────────────────────
// Configuration
// ──────────────────────────────────────────────
void VideoOptimizer::setPlaylistRoot(const QString &root)
{
    m_playlistRoot = root;
}

void VideoOptimizer::setOptimizedSuffix(const QString &suffix)
{
    m_optimizedSuffix = suffix;
}

void VideoOptimizer::setHandbrakePreset(const QString &preset)
{
    m_handbrakePreset = preset;
}

// ──────────────────────────────────────────────
// Locate HandBrakeCLI
// ──────────────────────────────────────────────
bool VideoOptimizer::findHandbrake()
{
    // Check common locations
    QStringList searchPaths;

#ifdef Q_OS_WIN
    searchPaths << "C:/Program Files/HandBrake/HandBrakeCLI.exe"
                << "C:/Program Files (x86)/HandBrake/HandBrakeCLI.exe";
#else
    searchPaths << "/usr/bin/HandBrakeCLI"
                << "/usr/local/bin/HandBrakeCLI";
#endif

    for (const QString &path : searchPaths) {
        if (QFileInfo::exists(path)) {
            m_handbrakePath = path;
            qInfo() << "[VideoOptimizer] Found HandBrakeCLI at:" << m_handbrakePath;
            return true;
        }
    }

    // Try PATH lookup
    const QString pathResult = QStandardPaths::findExecutable("HandBrakeCLI");
    if (!pathResult.isEmpty()) {
        m_handbrakePath = pathResult;
        qInfo() << "[VideoOptimizer] Found HandBrakeCLI in PATH:" << m_handbrakePath;
        return true;
    }

    qWarning() << "[VideoOptimizer] HandBrakeCLI not found. Video optimization disabled.";
    return false;
}

// ──────────────────────────────────────────────
// Start Optimization
// ──────────────────────────────────────────────
void VideoOptimizer::startOptimization()
{
    if (m_isOptimizing) {
        qWarning() << "[VideoOptimizer] Already optimizing, ignoring duplicate start";
        return;
    }

    m_cancelled = false;
    m_statusMessage = "Checking for HandBrakeCLI...";
    emit statusMessageChanged();

    if (!findHandbrake()) {
        m_statusMessage = "HandBrakeCLI not found — skipping optimization";
        emit statusMessageChanged();
        qInfo() << "[VideoOptimizer] No HandBrakeCLI found, emitting finished immediately";
        emit optimizationFinished();
        return;
    }

    m_statusMessage = "Scanning for unoptimized videos...";
    emit statusMessageChanged();

    scanForUnoptimizedFiles();

    if (m_jobQueue.isEmpty()) {
        m_statusMessage = "All videos already optimized";
        emit statusMessageChanged();
        qInfo() << "[VideoOptimizer] No files to optimize";
        emit optimizationFinished();
        return;
    }

    m_totalFiles = m_jobQueue.size();
    m_completedFiles = 0;
    m_isOptimizing = true;
    emit isOptimizingChanged();
    emit progressChanged();

    qInfo() << "[VideoOptimizer] Starting optimization of" << m_totalFiles << "files";
    processNextJob();
}

void VideoOptimizer::cancelOptimization()
{
    m_cancelled = true;

    if (m_process && m_process->state() != QProcess::NotRunning) {
        qInfo() << "[VideoOptimizer] Cancelling current HandBrake process...";
        m_process->kill();
        m_process->waitForFinished(5000);
    }

    m_jobQueue.clear();
    m_isOptimizing = false;
    emit isOptimizingChanged();
}

// ──────────────────────────────────────────────
// Scan for Files Needing Optimization
// ──────────────────────────────────────────────
void VideoOptimizer::scanForUnoptimizedFiles()
{
    m_jobQueue.clear();

    const QStringList zoneDirs = {
        "playlist-background",
        "playlist-main",
        "playlist-horizontal",
        "playlist-vertical"
    };

    for (const QString &zoneDir : zoneDirs) {
        const QString dirPath = m_playlistRoot + "/" + zoneDir;
        QDir dir(dirPath);

        if (!dir.exists()) {
            qDebug() << "[VideoOptimizer] Skipping non-existent dir:" << dirPath;
            continue;
        }

        QDirIterator it(dirPath, QDir::Files | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            it.next();
            const QFileInfo fi = it.fileInfo();
            const QString ext = fi.suffix().toLower();

            if (!isVideoFile(ext))
                continue;

            // Skip files that are already optimized output
            if (fi.completeBaseName().endsWith(m_optimizedSuffix))
                continue;

            // Check if optimized version already exists
            const QString outputPath = buildOutputPath(fi.absoluteFilePath());
            if (QFileInfo::exists(outputPath)) {
                qDebug() << "[VideoOptimizer] Already optimized:" << fi.fileName();
                continue;
            }

            m_jobQueue.enqueue({fi.absoluteFilePath(), outputPath});
        }
    }

    qInfo() << "[VideoOptimizer] Found" << m_jobQueue.size() << "files needing optimization";
}

// ──────────────────────────────────────────────
// Process Queue
// ──────────────────────────────────────────────
void VideoOptimizer::processNextJob()
{
    if (m_cancelled || m_jobQueue.isEmpty()) {
        m_isOptimizing = false;
        m_statusMessage = m_cancelled ? "Optimization cancelled" : "Optimization complete";
        emit isOptimizingChanged();
        emit statusMessageChanged();
        emit optimizationFinished();
        return;
    }

    const OptimizeJob job = m_jobQueue.dequeue();
    const QFileInfo fi(job.inputPath);

    m_statusMessage = QStringLiteral("Optimizing (%1/%2): %3")
                          .arg(m_completedFiles + 1)
                          .arg(m_totalFiles)
                          .arg(fi.fileName());
    emit statusMessageChanged();

    qInfo() << "[VideoOptimizer]" << m_statusMessage;

    // Build HandBrakeCLI arguments
    // Target: H.265 (HEVC), quality-based encoding for hardware decode on Pi
    QStringList args;
    args << "-i" << job.inputPath
         << "-o" << job.outputPath
         << "--preset" << m_handbrakePreset
         << "--encoder" << "x265"
         << "--quality" << "22"          // CRF 22 is a good balance
         << "--encoder-preset" << "medium"
         << "--no-markers"
         << "--optimize";

    qDebug() << "[VideoOptimizer] Running:" << m_handbrakePath << args;

    m_process->start(m_handbrakePath, args);
}

// ──────────────────────────────────────────────
// Process Event Handlers
// ──────────────────────────────────────────────
void VideoOptimizer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        qWarning() << "[VideoOptimizer] HandBrake exited with code" << exitCode;
        emit errorOccurred(QStringLiteral("HandBrake exited with code %1").arg(exitCode));
    } else {
        qInfo() << "[VideoOptimizer] File optimization complete";
    }

    m_completedFiles++;
    emit progressChanged();

    // Continue with next job
    processNextJob();
}

void VideoOptimizer::onProcessError(QProcess::ProcessError error)
{
    qCritical() << "[VideoOptimizer] QProcess error:" << error
                << m_process->errorString();
    emit errorOccurred("HandBrake process error: " + m_process->errorString());

    // Try next job despite the error
    m_completedFiles++;
    emit progressChanged();
    processNextJob();
}

void VideoOptimizer::onProcessOutput()
{
    // Read and log HandBrake output (useful for progress tracking)
    const QByteArray stdOut = m_process->readAllStandardOutput();
    const QByteArray stdErr = m_process->readAllStandardError();

    if (!stdOut.isEmpty()) {
        // HandBrake outputs progress info — parse percentage if desired
        const QString output = QString::fromUtf8(stdOut).trimmed();
        qDebug() << "[VideoOptimizer][stdout]" << output;
    }
    if (!stdErr.isEmpty()) {
        qDebug() << "[VideoOptimizer][stderr]" << QString::fromUtf8(stdErr).trimmed();
    }
}

// ──────────────────────────────────────────────
// Accessors
// ──────────────────────────────────────────────
bool VideoOptimizer::isOptimizing() const     { return m_isOptimizing; }
QString VideoOptimizer::statusMessage() const { return m_statusMessage; }
int VideoOptimizer::totalFiles() const        { return m_totalFiles; }
int VideoOptimizer::completedFiles() const    { return m_completedFiles; }

double VideoOptimizer::progress() const
{
    if (m_totalFiles <= 0) return 1.0;
    return static_cast<double>(m_completedFiles) / static_cast<double>(m_totalFiles);
}

// ──────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────
bool VideoOptimizer::isVideoFile(const QString &ext) const
{
    return s_videoExtensions.contains(ext);
}

QString VideoOptimizer::buildOutputPath(const QString &inputPath) const
{
    QFileInfo fi(inputPath);
    // e.g., video.mp4 → video_optimized.mp4
    return fi.absolutePath() + "/"
         + fi.completeBaseName() + m_optimizedSuffix
         + "." + fi.suffix();
}

bool VideoOptimizer::isAlreadyHevc(const QString & /*filePath*/) const
{
    // TODO: Use ffprobe or mediainfo to check codec
    // For now, rely on the filename-based optimized suffix check
    return false;
}
