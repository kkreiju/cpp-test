#ifndef WINDOWSERVICE_H
#define WINDOWSERVICE_H

#include <QObject>
#include <QQuickWindow>

/**
 * WindowService - Manages the Qt Quick window lifecycle.
 *
 * Singleton that controls window visibility, fullscreen mode,
 * cursor hiding (for kiosk), and provides window handle (winId)
 * to ZonePlayer for libVLC overlay attachment.
 */
class WindowService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool cursorVisible READ cursorVisible WRITE setCursorVisible NOTIFY cursorVisibleChanged)
    Q_PROPERTY(bool fullscreen    READ isFullscreen  NOTIFY fullscreenChanged)

public:
    static WindowService *instance();

    Q_INVOKABLE void setMainWindow(QQuickWindow* window);
    QQuickWindow *mainWindow() const;

    // Get native window ID for libVLC
    Q_INVOKABLE quintptr windowId() const;

    bool cursorVisible() const;
    void setCursorVisible(bool visible);
    bool isFullscreen() const;

    Q_INVOKABLE void toggleFullscreen();
    Q_INVOKABLE void hideCursor();
    Q_INVOKABLE void showCursor();

signals:
    void cursorVisibleChanged();
    void fullscreenChanged();

private:
    explicit WindowService(QObject *parent = nullptr);
    static WindowService *s_instance;

    QQuickWindow *m_window = nullptr;
    bool m_cursorVisible   = true;
};

#endif // WINDOWSERVICE_H
