#include "services/PidService.h"

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#else
#include <signal.h>
#endif

PidService::PidService(QObject *parent)
    : QObject(parent)
{
    m_pidPath = pidFilePath();
}

PidService::~PidService()
{
    if (m_acquired) {
        release();
    }
}

QString PidService::pidFilePath() const
{
#ifdef NCTV_PLATFORM_PI
    return QStringLiteral("/var/run/nctv-player.pid");
#else
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation)
           + QStringLiteral("/nctv-player.pid");
#endif
}

bool PidService::acquire()
{
    // Check if a PID file already exists
    QFile file(m_pidPath);
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            bool ok = false;
            qint64 existingPid = in.readLine().trimmed().toLongLong(&ok);
            file.close();

            if (ok && isProcessRunning(existingPid)) {
                qWarning() << "[PidService] Another instance is running (PID:" << existingPid << ")";
                return false;
            } else {
                qInfo() << "[PidService] Stale PID file found, removing";
                QFile::remove(m_pidPath);
            }
        }
    }

    // Write our PID
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << QCoreApplication::applicationPid();
        file.close();
        m_acquired = true;
        qInfo() << "[PidService] PID file created:" << m_pidPath
                << "(PID:" << QCoreApplication::applicationPid() << ")";
        return true;
    }

    qWarning() << "[PidService] Could not create PID file:" << m_pidPath;
    // Still allow running even if PID file fails (non-fatal)
    m_acquired = true;
    return true;
}

void PidService::release()
{
    if (m_acquired) {
        QFile::remove(m_pidPath);
        m_acquired = false;
        qInfo() << "[PidService] PID file removed";
    }
}

bool PidService::isRunning() const
{
    QFile file(m_pidPath);
    if (!file.exists()) return false;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        bool ok = false;
        qint64 pid = in.readLine().trimmed().toLongLong(&ok);
        file.close();
        return ok && isProcessRunning(pid);
    }
    return false;
}

bool PidService::isProcessRunning(qint64 pid) const
{
#ifdef Q_OS_WIN
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (process) {
        DWORD exitCode = 0;
        GetExitCodeProcess(process, &exitCode);
        CloseHandle(process);
        return (exitCode == STILL_ACTIVE);
    }
    return false;
#else
    // On Linux, kill(pid, 0) checks existence without actually sending a signal
    return (kill(static_cast<pid_t>(pid), 0) == 0);
#endif
}
