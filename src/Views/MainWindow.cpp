#include "MainWindow.h"

#include <QMouseEvent>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>
#include <QWindow>

#include "AdminOverviewView.h"
#include "DateNavigationTab.h"
#include "ItemListView.h"
#include "LoginDialog.h"
#include "ReportsView.h"
#include "SongsView.h"
#include "TitleBar.h"
#include "Style.h"

namespace {

constexpr int kResizeMargin = 6;

// The window's central widget: hosts TitleBar + the QStackedWidget of
// pages, with a thin margin around them so *this* widget (rather than a
// child) receives mouse events right at the window edge — used to drive
// OS-native interactive resize (QWindow::startSystemResize) since the
// window itself has no native frame to grab.
class ResizeFrame : public QWidget
{
public:
    explicit ResizeFrame(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("windowFrame"));
        setMouseTracking(true);
    }

protected:
    void mouseMoveEvent(QMouseEvent *event) override
    {
        setCursor(cursorForEdges(edgesAt(event->pos())));
        QWidget::mouseMoveEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            const Qt::Edges edges = edgesAt(event->pos());
            if (edges != Qt::Edges() && window()->windowHandle()) {
                window()->windowHandle()->startSystemResize(edges);
                event->accept();
                return;
            }
        }
        QWidget::mousePressEvent(event);
    }

private:
    Qt::Edges edgesAt(const QPoint &pos) const
    {
        Qt::Edges edges;
        if (pos.x() <= kResizeMargin) {
            edges |= Qt::LeftEdge;
        } else if (pos.x() >= width() - kResizeMargin) {
            edges |= Qt::RightEdge;
        }
        if (pos.y() <= kResizeMargin) {
            edges |= Qt::TopEdge;
        } else if (pos.y() >= height() - kResizeMargin) {
            edges |= Qt::BottomEdge;
        }
        return edges;
    }

    static Qt::CursorShape cursorForEdges(Qt::Edges edges)
    {
        const bool left = edges & Qt::LeftEdge;
        const bool right = edges & Qt::RightEdge;
        const bool top = edges & Qt::TopEdge;
        const bool bottom = edges & Qt::BottomEdge;
        if ((left && top) || (right && bottom)) {
            return Qt::SizeFDiagCursor;
        }
        if ((right && top) || (left && bottom)) {
            return Qt::SizeBDiagCursor;
        }
        if (left || right) {
            return Qt::SizeHorCursor;
        }
        if (top || bottom) {
            return Qt::SizeVerCursor;
        }
        return Qt::ArrowCursor;
    }
};

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(QStringLiteral("Potter's Portal"));
    resize(1080, 720);
    setMinimumSize(760, 480);

    m_itemListView = new ItemListView(&m_itemController, &m_tagController, &m_categoryController, this);
    m_adminOverviewView = new AdminOverviewView(
        &m_tagController, &m_categoryController, &m_dutyTypeController, &m_userController, &m_networkManager, this);
    m_dateNavigationTab = new DateNavigationTab(
        &m_dutyController, &m_userController, &m_dutyTypeController, &m_networkManager, this);
    m_songsView = new SongsView(&m_songController, this);
    m_reportsView = new ReportsView(&m_dutyController, &m_userController, this);

    auto *frame = new ResizeFrame(this);

    m_titleBar = new TitleBar(frame);
    // Date is inserted first (leftmost) per FR-8.1; the rest keep their
    // existing left-to-right order. Assigning a duty happens via a button
    // on the Date tab itself (see DateNavigationTab), not a separate tab.
    m_titleBar->tabBar()->insertTab(0, QStringLiteral("Date"));
    m_titleBar->tabBar()->addTab(QStringLiteral("Reports"));
    m_titleBar->tabBar()->addTab(QStringLiteral("Songs"));
    m_titleBar->tabBar()->addTab(QStringLiteral("Items"));
    m_titleBar->tabBar()->addTab(QStringLiteral("Taxonomy"));

    m_stack = new QStackedWidget(frame);
    m_stack->insertWidget(0, m_dateNavigationTab);
    m_stack->addWidget(m_reportsView);
    m_stack->addWidget(m_songsView);
    m_stack->addWidget(m_itemListView);
    m_stack->addWidget(m_adminOverviewView);

    connect(m_titleBar->tabBar(), &QTabBar::currentChanged, m_stack, &QStackedWidget::setCurrentIndex);
    connect(m_titleBar, &TitleBar::minimizeClicked, this, &QWidget::showMinimized);
    connect(m_titleBar, &TitleBar::closeClicked, this, &QWidget::close);
    connect(m_titleBar, &TitleBar::maximizeClicked, this, &MainWindow::toggleMaximizeRestore);
    connect(m_titleBar, &TitleBar::adminButtonClicked, this, &MainWindow::adminButtonClicked);
    connect(m_titleBar, &TitleBar::themeButtonClicked, this, &MainWindow::themeButtonClicked);
    m_titleBar->setBlackTheme(currentTheme() == Theme::Black);

    auto *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(kResizeMargin, kResizeMargin, kResizeMargin, kResizeMargin);
    frameLayout->setSpacing(0);
    frameLayout->addWidget(m_titleBar);
    frameLayout->addWidget(m_stack);

    setCentralWidget(frame);

    connect(&m_itemController, &ItemController::itemsChanged, m_itemListView, &ItemListView::refresh);
    connect(&m_tagController, &TagController::tagsChanged, m_itemListView, &ItemListView::refresh);
    connect(&m_categoryController, &CategoryController::categoriesChanged, m_itemListView, &ItemListView::refresh);

    connect(&m_categoryController, &CategoryController::categoriesChanged, m_adminOverviewView, &AdminOverviewView::refresh);
    connect(&m_tagController, &TagController::tagsChanged, m_adminOverviewView, &AdminOverviewView::refresh);
    connect(&m_dutyController, &DutyController::dutiesChanged, m_adminOverviewView, &AdminOverviewView::refresh);
    connect(&m_userController, &UserController::usersChanged, m_adminOverviewView, &AdminOverviewView::refresh);
    connect(&m_dutyTypeController, &DutyTypeController::dutyTypesChanged, m_adminOverviewView, &AdminOverviewView::refresh);

    connect(&m_dutyController, &DutyController::dutiesChanged, m_dateNavigationTab, &DateNavigationTab::refresh);
    connect(&m_userController, &UserController::usersChanged, m_dateNavigationTab, &DateNavigationTab::refresh);
    connect(&m_dutyTypeController, &DutyTypeController::dutyTypesChanged, m_dateNavigationTab, &DateNavigationTab::refresh);

    connect(&m_dutyController, &DutyController::dutiesChanged, m_reportsView, &ReportsView::refresh);
    connect(&m_userController, &UserController::usersChanged, m_reportsView, &ReportsView::refresh);
    connect(&m_dutyTypeController, &DutyTypeController::dutyTypesChanged, m_reportsView, &ReportsView::refresh);
}

void MainWindow::adminButtonClicked()
{
    if (m_currentUser.id() >= 0) {
        // Already logged in -- clicking the unlocked icon logs out.
        m_currentUser = User();
    } else {
        LoginDialog dialog(&m_userController, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        m_currentUser = dialog.loggedInUser();
    }

    const bool isAdmin = m_currentUser.id() >= 0;
    m_titleBar->setAdminLoggedIn(isAdmin);
    m_dateNavigationTab->setAdminMode(isAdmin);
    m_adminOverviewView->setAdminMode(isAdmin, m_currentUser.id());
    m_songsView->setAdminMode(isAdmin);
}

void MainWindow::themeButtonClicked()
{
    const Theme theme = currentTheme() == Theme::Black ? Theme::Light : Theme::Black;
    applyTheme(theme);
    m_titleBar->setBlackTheme(theme == Theme::Black);
    // The Sunday list's per-row colors and the report's HTML aren't
    // reached by the stylesheet.
    m_dateNavigationTab->restyleSundayItems();
    m_reportsView->restyleReport();
}

void MainWindow::toggleMaximizeRestore()
{
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && m_titleBar) {
        m_titleBar->setMaximized(isMaximized());
    }
}
