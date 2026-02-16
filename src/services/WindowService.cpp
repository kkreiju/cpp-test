#include "services/WindowService.h"

#include <QGuiApplication>
#include <QCursor>
#include <QDebug>

WindowService *WindowService::s_instance = nullptr;

WindowService::WindowService(QObject *parent)
    : QObject(parent)
{
}

WindowService *WindowService::instance()
{
    if (!s_instance) {
        s_instance = new WindowService(qApp);
    }
    return s_instance;
}

void WindowService::setMainWindow(QQuickWindow *window)
{
    m_window = window;
    if (m_window) {
        qInfo() << "[WindowService] Main window set. WinId:" << m_window->winId();
    }
}

QQuickWindow *WindowService::mainWindow() const
{
    return m_window;
}

quintptr WindowService::windowId() const
{
    return m_window ? m_window->winId() : 0;
}

bool WindowService::cursorVisible() const
{
    return m_cursorVisible;
}

void WindowService::setCursorVisible(bool visible)
{
    if (m_cursorVisible == visible) return;
    m_cursorVisible = visible;

    if (visible) {
        QGuiApplication::restoreOverrideCursor();
    } else {
        QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
    }

    emit cursorVisibleChanged();
    qDebug() << "[WindowService] Cursor visible:" << m_cursorVisible;
}

bool WindowService::isFullscreen() const
{
    return m_window && (m_window->visibility() == QWindow::FullScreen);
}

void WindowService::toggleFullscreen()
{
    if (!m_window) return;

    if (m_window->visibility() == QWindow::FullScreen) {
        m_window->showNormal();
    } else {
        m_window->showFullScreen();
    }

    emit fullscreenChanged();
}

void WindowService::hideCursor()
{
    setCursorVisible(false);
}

void WindowService::showCursor()
{
    setCursorVisible(true);
}
