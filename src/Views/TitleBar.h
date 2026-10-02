#pragma once

#include <QWidget>

class QTabBar;
class QToolButton;
class QLabel;

// Custom replacement for the OS title bar: app name on the left, the tab
// bar (Add Item / Items / Tags & Categories) in the same row, then
// minimize/maximize-restore/close buttons on the right. MainWindow owns the
// actual window-state changes; this widget only reports intent (clicked
// signals) plus handles dragging the window via a click-and-drag on any
// empty space in the bar.
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

    QTabBar *tabBar() const { return m_tabBar; }

    // Reflects window state so the maximize button shows the right
    // glyph/tooltip (restore vs. maximize).
    void setMaximized(bool maximized);

    // Swaps the lock icon (🔒 logged out / 🔓 admin) and its tooltip.
    void setAdminLoggedIn(bool loggedIn);

    // Shows the theme button's glyph for switching away from the current
    // theme: a moon while Light, a sun while Black.
    void setBlackTheme(bool isBlack);

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();
    void adminButtonClicked();
    void themeButtonClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *m_appIcon = nullptr;
    QTabBar *m_tabBar = nullptr;
    QToolButton *m_themeButton = nullptr;
    QToolButton *m_adminButton = nullptr;
    QToolButton *m_minimizeButton = nullptr;
    QToolButton *m_maximizeButton = nullptr;
    QToolButton *m_closeButton = nullptr;
    bool m_maximized = false;
};
