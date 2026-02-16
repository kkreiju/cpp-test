#ifndef VIDEOOPTIMIZER_H
#define VIDEOOPTIMIZER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QQueue>

/**
 * VideoOptimizer - Background HandBrakeCLI wrapper for H.265 (HEVC) transcoding.
 *
 * On startup, scans the playlist directories for video files that haven't
 * been optimized yet. Queues them up and spawns HandBrakeCLI sequentially
 * via QProcess. The Splash screen stays visible until optimization finishes.
 *
 * Target: H.265 (HEVC) for native 4K hardware decoding on Raspberry Pi.
 */
class VideoOptimizer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool    isOptimizing   READ isOptimizing   NOTIFY isOptimizingChanged)
    Q_PROPERTY(QString statusMessage  READ statusMessage  NOTIFY statusMessageChanged)
    Q_PROPERTY(int     totalFiles     READ totalFiles     NOTIFY progressChanged)
    Q_PROPERTY(int     completedFiles READ completedFiles NOTIFY progressChanged)
    Q_PROPERTY(double  progress       READ progress       NOTIFY progressChanged)

public:
    explicit VideoOptimizer(QObject *parent = nullptr);
    ~VideoOptimizer() override;

    // ── Configuration ──
    void setPlaylistRoot(const QString &root);
    void setOptimizedSuffix(const QString &suffix);
    void setHandbrakePreset(const QString &preset);

    // ── Control ──
    Q_INVOKABLE void startOptimization();
    Q_INVOKABLE void cancelOptimization();

    // ── Accessors ──
    bool    isOptimizing() const;
    QString statusMessage() const;
    int     totalFiles() const;
    int     completedFiles() const;
    double  progress() const;

signals:
    void isOptimizingChanged();
    void statusMessageChanged();
    void progressChanged();
    void optimizationFinished();
    void fileOptimized(const QString &inputPath, const QString &outputPath);
    void errorOccurred(const QString &message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessOutput();

private:
    struct OptimizeJob {
        QString inputPath;
        QString outputPath;
    };

    void scanForUnoptimizedFiles();
    void processNextJob();
    bool isVideoFile(const QString &ext) const;
    bool isAlreadyHevc(const QString &filePath) const;
    QString buildOutputPath(const QString &inputPath) const;
    bool findHandbrake();

    QString     m_playlistRoot;
    QString     m_optimizedSuffix = "_optimized";
    QString     m_handbrakePreset = "H.265 MKV 1080p30"; // Default HandBrake preset
    QString     m_handbrakePath;

    QProcess   *m_process         = nullptr;
    bool        m_isOptimizing    = false;
    bool        m_cancelled       = false;
    QString     m_statusMessage   = "Initializing...";

    QQueue<OptimizeJob> m_jobQueue;
    int         m_totalFiles      = 0;
    int         m_completedFiles  = 0;

    static const QStringList s_videoExtensions;
};

#endif // VIDEOOPTIMIZER_H
