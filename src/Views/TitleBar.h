#pragma once

#include <QWidget>

class QToolButton;
class QLabel;

// Custom replacement for the OS title bar: app icon and name on the left,
// then the admin lock and minimize/maximize-restore/close buttons on the
// right. The theme and password change live on the Settings page. Page navigation lives in the Sidebar below it. MainWindow owns the
// actual window-state changes; this widget only reports intent (clicked
// signals) plus handles dragging the window via a click-and-drag on any
// empty space in the bar.
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

    // Reflects window state so the maximize button shows the right
    // glyph/tooltip (restore vs. maximize).
    void setMaximized(bool maximized);

    // Swaps the lock icon (🔒 logged out / 🔓 admin) and its tooltip.
    void setAdminLoggedIn(bool loggedIn);

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();
    void adminButtonClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *m_appIcon = nullptr;
    QLabel *m_appName = nullptr;
    QToolButton *m_adminButton = nullptr;
    QToolButton *m_minimizeButton = nullptr;
    QToolButton *m_maximizeButton = nullptr;
    QToolButton *m_closeButton = nullptr;
    bool m_maximized = false;
};
