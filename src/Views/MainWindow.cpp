#include "MainWindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWindow>

#include "AccessRightsDialog.h"
#include "AdminOverviewView.h"
#include "ScheduleTab.h"
#include "ItemListView.h"
#include "ChangePasswordDialog.h"
#include "LoginDialog.h"
#include "ReportsView.h"
#include "SettingsView.h"
#include "SongsView.h"
#include "FeedbackView.h"
#include "Sidebar.h"
#include "TitleBar.h"
#include "Language.h"
#include "Style.h"

namespace {

constexpr int kResizeMargin = 6;

// The window's central widget: hosts TitleBar, Sidebar + the QStackedWidget of
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
    setWindowTitle(tr("Potters Portal"));
    resize(1080, 720);
    // No fixed minimum: the window can't be made smaller than its pages
    // need (e.g. a Schedule row), so nothing gets cut off at the edge.

    m_itemListView = new ItemListView(&m_itemController, &m_tagController, &m_categoryController, this);
    m_adminOverviewView = new AdminOverviewView(
        &m_tagController, &m_categoryController, &m_dutyTypeController, &m_userController, &m_teamController, this);
    m_scheduleTab = new ScheduleTab(
        &m_dutyController, &m_userController, &m_dutyTypeController, &m_teamController, this);
    m_songsView = new SongsView(&m_songController, this);
    m_feedbackView = new FeedbackView(&m_feedbackController, &m_userController, this);
    m_reportsView = new ReportsView(&m_dutyController, &m_userController, &m_dutyTypeController, &m_teamController, this);
    m_settingsView = new SettingsView(this);
    m_settingsView->setCurrentTheme(currentTheme());
    m_settingsView->setCurrentLanguage(savedLanguage());

    auto *frame = new ResizeFrame(this);

    m_titleBar = new TitleBar(frame);

    // Schedule comes first (top) per FR-8.1; the rest keep their existing
    // order. Assigning a duty happens via a button on the Schedule page
    // itself (see ScheduleTab), not a separate page. Sidebar order must
    // match the stack's.
    m_sidebar = new Sidebar(frame);
    m_sidebar->addPage(QStringLiteral("📅"), tr("Schedule")); // 📅
    m_sidebar->addPage(QStringLiteral("📊"), tr("Reports"));  // 📊
    m_sidebar->addPage(QStringLiteral("🎵"), tr("Songs"));    // 🎵
    m_sidebar->addPage(QStringLiteral("📦"), tr("Inventory"));    // 📦
    m_sidebar->addPage(QStringLiteral("🏷"), tr("Taxonomy")); // 🏷
    m_sidebar->addPage(QStringLiteral("💬"), tr("Feedback"));
    m_sidebar->addPage(QStringLiteral("⚙"), tr("Settings"));     // ⚙

    m_stack = new QStackedWidget(frame);
    m_stack->addWidget(m_scheduleTab);
    m_stack->addWidget(m_reportsView);
    m_stack->addWidget(m_songsView);
    m_stack->addWidget(m_itemListView);
    m_stack->addWidget(m_adminOverviewView);
    m_stack->addWidget(m_feedbackView);
    m_stack->addWidget(m_settingsView);

    connect(m_sidebar, &Sidebar::currentChanged, m_stack, &QStackedWidget::setCurrentIndex);
    connect(m_titleBar, &TitleBar::minimizeClicked, this, &QWidget::showMinimized);
    connect(m_titleBar, &TitleBar::closeClicked, this, &QWidget::close);
    connect(m_titleBar, &TitleBar::maximizeClicked, this, &MainWindow::toggleMaximizeRestore);
    connect(m_titleBar, &TitleBar::adminButtonClicked, this, &MainWindow::adminButtonClicked);
    connect(m_settingsView, &SettingsView::changePasswordClicked, this, &MainWindow::changePasswordClicked);
    connect(m_settingsView, &SettingsView::themeChosen, this, &MainWindow::themeChosen);
    connect(m_settingsView, &SettingsView::languageChosen, this, &MainWindow::languageChosen);
    connect(m_settingsView, &SettingsView::accessRightsClicked, this, &MainWindow::accessRightsClicked);

    // A duty's rights follow who has it on the upcoming Sunday, and a
    // member's team can change, so recheck whenever any of it does.
    connect(&m_accessController, &AccessController::rulesChanged, this, &MainWindow::applyAccess);
    connect(&m_dutyController, &DutyController::dutiesChanged, this, &MainWindow::applyAccess);
    connect(&m_userController, &UserController::usersChanged, this, &MainWindow::applyAccess);

    auto *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(kResizeMargin, kResizeMargin, kResizeMargin, kResizeMargin);
    frameLayout->setSpacing(0);
    frameLayout->addWidget(m_titleBar);
    auto *body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);
    body->addWidget(m_sidebar);
    body->addWidget(m_stack, 1);
    frameLayout->addLayout(body, 1);

    setCentralWidget(frame);

    connect(&m_itemController, &ItemController::itemsChanged, m_itemListView, &ItemListView::refresh);
    connect(&m_tagController, &TagController::tagsChanged, m_itemListView, &ItemListView::refresh);
    connect(&m_categoryController, &CategoryController::categoriesChanged, m_itemListView, &ItemListView::refresh);

    connect(&m_categoryController, &CategoryController::categoriesChanged, m_adminOverviewView, &AdminOverviewView::refresh);
    connect(&m_tagController, &TagController::tagsChanged, m_adminOverviewView, &AdminOverviewView::refresh);
    connect(&m_dutyController, &DutyController::dutiesChanged, m_adminOverviewView, &AdminOverviewView::refresh);
    connect(&m_userController, &UserController::usersChanged, m_adminOverviewView, &AdminOverviewView::refresh);
    connect(&m_dutyTypeController, &DutyTypeController::dutyTypesChanged, m_adminOverviewView, &AdminOverviewView::refresh);
    connect(&m_teamController, &TeamController::teamsChanged, m_adminOverviewView, &AdminOverviewView::refresh);

    connect(&m_dutyController, &DutyController::dutiesChanged, m_scheduleTab, &ScheduleTab::refresh);
    connect(&m_userController, &UserController::usersChanged, m_scheduleTab, &ScheduleTab::refresh);
    connect(&m_dutyTypeController, &DutyTypeController::dutyTypesChanged, m_scheduleTab, &ScheduleTab::refresh);

    connect(&m_dutyController, &DutyController::dutiesChanged, m_reportsView, &ReportsView::refresh);
    connect(&m_userController, &UserController::usersChanged, m_reportsView, &ReportsView::refresh);
    connect(&m_userController, &UserController::usersChanged, m_feedbackView, &FeedbackView::refresh);
    connect(&m_dutyTypeController, &DutyTypeController::dutyTypesChanged, m_reportsView, &ReportsView::refresh);
    connect(&m_teamController, &TeamController::teamsChanged, m_reportsView, &ReportsView::refresh);

    applyAccess();
}

void MainWindow::adminButtonClicked()
{
    if (m_currentUser.id() >= 0) {
        // Already signed in -- clicking the unlocked icon signs out.
        m_currentUser = User();
    } else {
        LoginDialog dialog(&m_userController, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        m_currentUser = dialog.loggedInUser();
    }
    applyAccess();
}

void MainWindow::applyAccess()
{
    // Re-read the account: their team or Admin role may have changed.
    if (m_currentUser.id() >= 0) {
        const User fresh = m_userController.userById(m_currentUser.id());
        m_currentUser = fresh.id() >= 0 ? fresh : User();
    }
    const bool isAdmin = m_currentUser.id() >= 0 && m_currentUser.isAdmin();
    const QDate today = QDate::currentDate();
    const QDate upcomingSunday = today.addDays(7 - today.dayOfWeek()); // today, if it's Sunday
    const AccessRights rights = m_accessController.rightsFor(m_currentUser, upcomingSunday);

    m_titleBar->setSignedIn(m_currentUser.id() >= 0 ? m_currentUser.name() : QString(), isAdmin);
    m_scheduleTab->setAdminMode(isAdmin);
    m_reportsView->setCanSave(rights.section(Section::Reports).create);
    m_songsView->setAccess(rights.section(Section::Songs));
    m_itemListView->setAccess(rights.section(Section::Inventory));
    m_adminOverviewView->setAccess(rights.section(Section::Taxonomy), isAdmin, m_currentUser.id());
    m_feedbackView->setAccess(rights.section(Section::Feedback));
    m_settingsView->setAdminMode(isAdmin);

    // Sidebar pages after Schedule, in the same order as allSections().
    const QVector<Section> sections = allSections();
    for (int i = 0; i < sections.size(); ++i) {
        m_sidebar->setPageVisible(i + 1, rights.section(sections[i]).view);
    }
    const int current = m_sidebar->currentIndex();
    if (current > 0 && !rights.section(sections[current - 1]).view) {
        m_sidebar->setCurrentIndex(0);
    }
}

void MainWindow::accessRightsClicked()
{
    if (!m_currentUser.isAdmin()) {
        return;
    }
    AccessRightsDialog dialog(&m_accessController, &m_userController, &m_teamController, &m_dutyTypeController, this);
    dialog.exec();
}

void MainWindow::changePasswordClicked()
{
    if (m_currentUser.id() < 0) {
        return;
    }
    // Re-read the account so the email goes to its current address.
    const User admin = m_userController.userById(m_currentUser.id());
    if (admin.id() < 0) {
        return;
    }
    ChangePasswordDialog dialog(admin, &m_userController, this);
    dialog.exec();
}

void MainWindow::themeChosen(Theme theme)
{
    applyTheme(theme);
    // The Sunday list's per-row colors and the report's HTML aren't
    // reached by the stylesheet.
    m_scheduleTab->restyleSundayItems();
    m_reportsView->restyleReport();
}

void MainWindow::languageChosen(Language language)
{
    if (language == savedLanguage()) {
        return;
    }
    saveLanguage(language);
    // Asked in the language being switched to, since that's the one the
    // person picked and can read.
    const bool french = language == Language::French;
    QMessageBox box(QMessageBox::Question,
        french ? QStringLiteral("Langue") : QStringLiteral("Language"),
        french ? QStringLiteral("Redémarrer Potters Portal en français maintenant ?")
               : QStringLiteral("Restart Potters Portal in English now?"),
        QMessageBox::NoButton, this);
    QPushButton *restartButton = box.addButton(
        french ? QStringLiteral("Redémarrer") : QStringLiteral("Restart"), QMessageBox::AcceptRole);
    box.addButton(french ? QStringLiteral("Plus tard") : QStringLiteral("Later"), QMessageBox::RejectRole);
    box.setInformativeText(french
        ? QStringLiteral("Sinon, le changement s'appliquera au prochain démarrage.")
        : QStringLiteral("Otherwise it applies the next time the app starts."));
    box.exec();
    if (box.clickedButton() != restartButton) {
        return;
    }
    if (QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments().mid(1))) {
        QApplication::quit();
    }
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
