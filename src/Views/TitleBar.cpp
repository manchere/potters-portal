#include "TitleBar.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QToolButton>
#include <QWindow>

namespace {

// Loads the app icon and clips it to a circle so it reads as a small app
// mark rather than a stray square thumbnail.
QPixmap loadAppIcon(int size)
{
    QPixmap source(QCoreApplication::applicationDirPath() + QStringLiteral("/pottershouse.jpg"));
    if (source.isNull()) {
        return QPixmap();
    }
    source = source.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap rounded(size, size);
    rounded.fill(Qt::transparent);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addEllipse(0, 0, size, size);
    painter.setClipPath(clip);
    const int x = (size - source.width()) / 2;
    const int y = (size - source.height()) / 2;
    painter.drawPixmap(x, y, source);
    return rounded;
}

} // namespace

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(44);

    m_appIcon = new QLabel(this);
    m_appIcon->setPixmap(loadAppIcon(28));
    m_appIcon->setFixedSize(28, 28);
    m_appIcon->setToolTip(QStringLiteral("Potter's Portal"));

    m_appName = new QLabel(QStringLiteral("Potter's Portal"), this);
    m_appName->setObjectName(QStringLiteral("titleBarAppName"));

    m_adminButton = new QToolButton(this);
    m_adminButton->setObjectName(QStringLiteral("titleBarButton"));
    connect(m_adminButton, &QToolButton::clicked, this, &TitleBar::adminButtonClicked);
    setAdminLoggedIn(false);

    m_minimizeButton = new QToolButton(this);
    m_minimizeButton->setObjectName(QStringLiteral("titleBarButton"));
    m_minimizeButton->setText(QStringLiteral("─"));
    m_minimizeButton->setToolTip(QStringLiteral("Minimize"));
    connect(m_minimizeButton, &QToolButton::clicked, this, &TitleBar::minimizeClicked);

    m_maximizeButton = new QToolButton(this);
    m_maximizeButton->setObjectName(QStringLiteral("titleBarButton"));
    m_maximizeButton->setToolTip(QStringLiteral("Maximize"));
    connect(m_maximizeButton, &QToolButton::clicked, this, &TitleBar::maximizeClicked);

    m_closeButton = new QToolButton(this);
    m_closeButton->setObjectName(QStringLiteral("titleBarCloseButton"));
    m_closeButton->setText(QStringLiteral("✕"));
    m_closeButton->setToolTip(QStringLiteral("Close"));
    connect(m_closeButton, &QToolButton::clicked, this, &TitleBar::closeClicked);

    setMaximized(false);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 8, 0);
    layout->setSpacing(4);
    layout->addWidget(m_appIcon);
    layout->addSpacing(8);
    layout->addWidget(m_appName);
    layout->addStretch();
    layout->addWidget(m_adminButton);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);
}

void TitleBar::setAdminLoggedIn(bool loggedIn)
{
    m_adminButton->setText(loggedIn ? QStringLiteral("\U0001F513") : QStringLiteral("\U0001F512")); // 🔓 / 🔒
    m_adminButton->setToolTip(loggedIn ? QStringLiteral("Admin mode -- click to log out") : QStringLiteral("Admin Login"));
}

void TitleBar::setMaximized(bool maximized)
{
    m_maximized = maximized;
    m_maximizeButton->setText(maximized ? QStringLiteral("❏") : QStringLiteral("□"));
    m_maximizeButton->setToolTip(maximized ? QStringLiteral("Restore") : QStringLiteral("Maximize"));
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    // Only reached for clicks on empty title bar space (or the app name) —
    // the buttons are child widgets and handle their own
    // mouse events first.
    if (event->button() == Qt::LeftButton) {
        if (QWindow *handle = window()->windowHandle()) {
            handle->startSystemMove();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeClicked();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}
