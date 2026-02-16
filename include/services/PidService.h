#ifndef PIDSERVICE_H
#define PIDSERVICE_H

#include <QObject>
#include <QString>

/**
 * PidService - Single-instance enforcement via PID file.
 *
 * Creates a lock file at /var/run/nctv-player.pid (Pi) or
 * a temp directory on desktop. Prevents multiple instances
 * of the player from running simultaneously.
 */
class PidService : public QObject
{
    Q_OBJECT

public:
    explicit PidService(QObject *parent = nullptr);
    ~PidService() override;

    /// Acquire the PID lock. Returns false if another instance is running.
    bool acquire();

    /// Release the PID lock.
    void release();

    /// Check if another instance is running (without acquiring).
    bool isRunning() const;

private:
    QString pidFilePath() const;
    bool isProcessRunning(qint64 pid) const;

    QString m_pidPath;
    bool    m_acquired = false;
};

#endif // PIDSERVICE_H
