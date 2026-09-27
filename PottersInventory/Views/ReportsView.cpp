#include "ReportsView.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>

#include "Controllers/AssignmentController.h"
#include "Controllers/UserController.h"

namespace
{
    // Marks the "Summary" entry in the Sunday list (Sunday rows store
    // their QDate instead).
    const QString kSummaryKey = QStringLiteral("summary");

    QString formatSunday(const QDate &date)
    {
        // Same format as the Date tab, e.g. "Sun 6 Sep 2026".
        return date.toString(QStringLiteral("ddd d MMM yyyy"));
    }

    QString esc(const QString &text)
    {
        return text.toHtmlEscaped();
    }

    QString memberOrUnfilled(const QString &name)
    {
        return name.isEmpty() ? QStringLiteral("<span class='muted'>Unfilled</span>") : esc(name);
    }

    // Human-readable status for the report's Status column.
    QString statusText(const ScheduleReportRow &row)
    {
        if (row.requestStatus == QLatin1String("approved")) {
            return QStringLiteral("Time off approved");
        }
        if (row.requestStatus == QLatin1String("pending")) {
            return QStringLiteral("Time off requested");
        }
        if (row.requestStatus == QLatin1String("denied")) {
            return QStringLiteral("Time off denied");
        }
        if (row.memberMarkedUnavailable) {
            return QStringLiteral("Marked unavailable");
        }
        return QString();
    }

    QString csvField(QString value)
    {
        if (value.contains(QLatin1Char(',')) || value.contains(QLatin1Char('"')) || value.contains(QLatin1Char('\n'))) {
            value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
            return QLatin1Char('"') + value + QLatin1Char('"');
        }
        return value;
    }
}

ReportsView::ReportsView(AssignmentController *assignmentController, UserController *userController, QWidget *parent)
    : QWidget(parent)
    , m_assignmentController(assignmentController)
    , m_userController(userController)
{
    auto *title = new QLabel(QStringLiteral("Reports"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("Look up past Sunday schedules. Pick a date range, then a Sunday to see who served, "
                        "or the summary to see the whole period."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    const QDate today = QDate::currentDate();
    m_fromEdit = new QDateEdit(today.addMonths(-3), this);
    m_toEdit = new QDateEdit(today, this);
    for (QDateEdit *edit : {m_fromEdit, m_toEdit}) {
        edit->setCalendarPopup(true);
        edit->setDisplayFormat(QStringLiteral("d MMM yyyy"));
        connect(edit, &QDateEdit::dateChanged, this, &ReportsView::reloadReport);
    }

    m_memberCombo = new QComboBox(this);
    m_memberCombo->setMinimumWidth(180);
    connect(m_memberCombo, &QComboBox::currentIndexChanged, this, &ReportsView::reloadReport);

    auto *filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel(QStringLiteral("From"), this));
    filterRow->addWidget(m_fromEdit);
    filterRow->addWidget(new QLabel(QStringLiteral("To"), this));
    filterRow->addWidget(m_toEdit);
    filterRow->addSpacing(12);
    filterRow->addWidget(new QLabel(QStringLiteral("Member"), this));
    filterRow->addWidget(m_memberCombo);
    filterRow->addSpacing(12);
    const QList<QPair<QString, int>> presets = {
        {QStringLiteral("Last month"), 1},
        {QStringLiteral("Last 3 months"), 3},
        {QStringLiteral("Last year"), 12},
    };
    for (const auto &preset : presets) {
        auto *button = new QPushButton(preset.first, this);
        button->setObjectName(QStringLiteral("secondaryButton"));
        const int months = preset.second;
        connect(button, &QPushButton::clicked, this, [this, months]() { presetClicked(months); });
        filterRow->addWidget(button);
    }
    filterRow->addStretch();

    m_sundayList = new QListWidget(this);
    m_sundayList->setFixedWidth(230);
    connect(m_sundayList, &QListWidget::currentRowChanged, this, &ReportsView::selectionChanged);
    auto *sundayBox = new QGroupBox(QStringLiteral("Sundays"), this);
    auto *sundayLayout = new QVBoxLayout(sundayBox);
    sundayLayout->addWidget(m_sundayList);

    m_reportView = new QTextBrowser(this);
    m_reportView->setOpenLinks(false);
    m_saveButton = new QPushButton(QStringLiteral("Save Report..."), this);
    m_saveButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_saveButton, &QPushButton::clicked, this, &ReportsView::saveClicked);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #666;"));
    auto *reportButtons = new QHBoxLayout;
    reportButtons->addWidget(m_statusLabel, 1);
    reportButtons->addWidget(m_saveButton);
    auto *reportBox = new QGroupBox(QStringLiteral("Report"), this);
    auto *reportLayout = new QVBoxLayout(reportBox);
    reportLayout->addWidget(m_reportView, 1);
    reportLayout->addLayout(reportButtons);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(sundayBox);
    columns->addWidget(reportBox, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(6);
    layout->addLayout(filterRow);
    layout->addLayout(columns, 1);

    refresh();
}

void ReportsView::refresh()
{
    populateMemberFilter();
    reloadReport();
}

void ReportsView::populateMemberFilter()
{
    const int previous = m_memberCombo->currentData().toInt();
    m_memberCombo->blockSignals(true);
    m_memberCombo->clear();
    m_memberCombo->addItem(QStringLiteral("All members"), -1);
    for (const User &user : m_userController->allUsers()) {
        m_memberCombo->addItem(user.name(), user.id());
    }
    const int index = m_memberCombo->findData(previous);
    m_memberCombo->setCurrentIndex(index >= 0 ? index : 0);
    m_memberCombo->blockSignals(false);
}

void ReportsView::presetClicked(int months)
{
    const QDate today = QDate::currentDate();
    m_fromEdit->blockSignals(true);
    m_toEdit->blockSignals(true);
    m_fromEdit->setDate(today.addMonths(-months));
    m_toEdit->setDate(today);
    m_fromEdit->blockSignals(false);
    m_toEdit->blockSignals(false);
    reloadReport();
}

void ReportsView::reloadReport()
{
    QDate from = m_fromEdit->date();
    QDate to = m_toEdit->date();
    if (from > to) {
        std::swap(from, to);
    }
    m_rows = m_assignmentController->scheduleReport(from, to, m_memberCombo->currentData().toInt());
    populateSundayList();
}

void ReportsView::populateSundayList()
{
    const QDate previouslySelected = selectedSunday();

    m_sundayList->blockSignals(true);
    m_sundayList->clear();
    auto *summary = new QListWidgetItem(QStringLiteral("Summary"), m_sundayList);
    summary->setData(Qt::UserRole, kSummaryKey);
    QFont bold = summary->font();
    bold.setBold(true);
    summary->setFont(bold);

    // m_rows is newest first, so Sundays come out newest first too.
    QMap<QDate, int> countByDate;
    QVector<QDate> dates;
    for (const ScheduleReportRow &row : m_rows) {
        if (!countByDate.contains(row.serviceDate)) {
            dates.append(row.serviceDate);
        }
        ++countByDate[row.serviceDate];
    }
    int selectRow = 0;
    for (const QDate &date : dates) {
        const int count = countByDate.value(date);
        auto *item = new QListWidgetItem(
            QStringLiteral("%1  ·  %2 %3").arg(formatSunday(date)).arg(count)
                .arg(count == 1 ? QStringLiteral("role") : QStringLiteral("roles")),
            m_sundayList);
        item->setData(Qt::UserRole, date);
        if (date == previouslySelected) {
            selectRow = m_sundayList->count() - 1;
        }
    }
    m_sundayList->setCurrentRow(selectRow);
    m_sundayList->blockSignals(false);

    selectionChanged();
}

QDate ReportsView::selectedSunday() const
{
    const QListWidgetItem *item = m_sundayList->currentItem();
    if (!item || item->data(Qt::UserRole).toString() == kSummaryKey) {
        return QDate();
    }
    return item->data(Qt::UserRole).toDate();
}

QVector<ScheduleReportRow> ReportsView::visibleRows() const
{
    const QDate sunday = selectedSunday();
    if (!sunday.isValid()) {
        return m_rows;
    }
    QVector<ScheduleReportRow> rows;
    for (const ScheduleReportRow &row : m_rows) {
        if (row.serviceDate == sunday) {
            rows.append(row);
        }
    }
    return rows;
}

QString ReportsView::filterDescription() const
{
    QDate from = m_fromEdit->date();
    QDate to = m_toEdit->date();
    if (from > to) {
        std::swap(from, to);
    }
    QString text = QStringLiteral("%1 – %2").arg(from.toString(QStringLiteral("d MMM yyyy")), to.toString(QStringLiteral("d MMM yyyy")));
    if (m_memberCombo->currentData().toInt() > 0) {
        text += QStringLiteral("  ·  %1").arg(m_memberCombo->currentText());
    }
    return text;
}

void ReportsView::selectionChanged()
{
    const QDate sunday = selectedSunday();
    const QString body = sunday.isValid() ? sundayHtml(sunday) : summaryHtml();
    m_reportView->setHtml(wrapHtml(sunday.isValid() ? formatSunday(sunday) : QStringLiteral("Schedule summary"), body));

    const int sundays = m_sundayList->count() - 1;
    m_statusLabel->setText(sundays == 0
        ? QStringLiteral("No schedules in this range.")
        : QStringLiteral("%1 Sunday(s) with a schedule in this range.").arg(sundays));
    m_saveButton->setEnabled(!m_rows.isEmpty());
}

QString ReportsView::wrapHtml(const QString &title, const QString &body) const
{
    // Kept to the subset of CSS QTextBrowser understands, so the saved
    // file and the on-screen view look alike.
    return QStringLiteral(
        "<html><head><meta charset='utf-8'><title>%1</title>"
        "<style>"
        "body { font-family: 'Segoe UI', sans-serif; color: #1f2430; }"
        "h1 { font-size: 20px; margin-bottom: 2px; }"
        "h2 { font-size: 15px; margin-top: 18px; margin-bottom: 6px; }"
        ".muted { color: #888; }"
        ".warn { color: #a8701c; }"
        "table { border-collapse: collapse; }"
        "th { text-align: left; background: #eef1f5; padding: 6px 10px; }"
        "td { padding: 6px 10px; border-bottom: 1px solid #e3e6ea; }"
        "</style></head><body>"
        "<h1>%1</h1><p class='muted'>%2</p>%3</body></html>")
        .arg(esc(title), esc(filterDescription()), body);
}

QString ReportsView::sundayHtml(const QDate &date) const
{
    const QVector<ScheduleReportRow> rows = visibleRows();
    QSet<int> serving;
    int unfilled = 0;
    for (const ScheduleReportRow &row : rows) {
        if (row.memberId > 0) {
            serving.insert(row.memberId);
        } else {
            ++unfilled;
        }
        if (row.supportMemberId > 0) {
            serving.insert(row.supportMemberId);
        }
    }

    QString html = QStringLiteral("<p><b>%1</b> role(s) &nbsp;·&nbsp; <b>%2</b> member(s) serving")
        .arg(rows.size()).arg(serving.size());
    if (unfilled > 0) {
        html += QStringLiteral(" &nbsp;·&nbsp; <span class='warn'><b>%1</b> unfilled</span>").arg(unfilled);
    }
    html += QStringLiteral("</p>");
    if (date > QDate::currentDate()) {
        html += QStringLiteral("<p class='warn'>This Sunday hasn't happened yet, so this is the current plan.</p>");
    }

    html += QStringLiteral("<table width='100%'><tr><th>Role</th><th>Serving</th><th>Support</th><th>Notes</th><th>Status</th></tr>");
    for (const ScheduleReportRow &row : rows) {
        const QString status = statusText(row);
        html += QStringLiteral("<tr><td>%1 %2</td><td>%3</td><td>%4</td><td>%5</td><td class='warn'>%6</td></tr>")
            .arg(esc(row.roleIcon), esc(row.roleName), memberOrUnfilled(row.memberName),
                 row.supportMemberId > 0 ? esc(row.supportMemberName) : QStringLiteral("<span class='muted'>—</span>"),
                 esc(row.notes), esc(status));
    }
    html += QStringLiteral("</table>");
    return html;
}

QString ReportsView::summaryHtml() const
{
    if (m_rows.isEmpty()) {
        return QStringLiteral("<p class='muted'>No Sunday schedules in this date range. "
                              "Try a wider range with the buttons above.</p>");
    }

    struct MemberTally { QString name; int primary = 0; int support = 0; };
    QMap<int, MemberTally> members;
    QMap<QString, int> roles;
    QSet<QDate> sundays;
    int unfilled = 0;
    int pending = 0;
    int approved = 0;
    int denied = 0;
    int markedUnavailable = 0;
    for (const ScheduleReportRow &row : m_rows) {
        sundays.insert(row.serviceDate);
        ++roles[row.roleIcon + QStringLiteral(" ") + row.roleName];
        if (row.memberId > 0) {
            members[row.memberId].name = row.memberName;
            ++members[row.memberId].primary;
        } else {
            ++unfilled;
        }
        if (row.supportMemberId > 0) {
            members[row.supportMemberId].name = row.supportMemberName;
            ++members[row.supportMemberId].support;
        }
        if (row.requestStatus == QLatin1String("pending")) {
            ++pending;
        } else if (row.requestStatus == QLatin1String("approved")) {
            ++approved;
        } else if (row.requestStatus == QLatin1String("denied")) {
            ++denied;
        } else if (row.memberMarkedUnavailable) {
            ++markedUnavailable;
        }
    }

    QString html = QStringLiteral(
        "<table><tr><td><b>%1</b><br><span class='muted'>Sundays</span></td>"
        "<td><b>%2</b><br><span class='muted'>assignments</span></td>"
        "<td><b>%3</b><br><span class='muted'>members serving</span></td>"
        "<td><b>%4</b><br><span class='muted'>unfilled slots</span></td></tr></table>")
        .arg(sundays.size()).arg(m_rows.size()).arg(members.size()).arg(unfilled);

    if (pending + approved + denied + markedUnavailable > 0) {
        html += QStringLiteral("<h2>Availability</h2><p>");
        QStringList parts;
        if (approved > 0) {
            parts << QStringLiteral("%1 time-off request(s) approved").arg(approved);
        }
        if (pending > 0) {
            parts << QStringLiteral("%1 still pending").arg(pending);
        }
        if (denied > 0) {
            parts << QStringLiteral("%1 denied").arg(denied);
        }
        if (markedUnavailable > 0) {
            parts << QStringLiteral("%1 assigned on a day they marked unavailable").arg(markedUnavailable);
        }
        html += esc(parts.join(QStringLiteral(" · "))) + QStringLiteral("</p>");
    }

    QVector<MemberTally> tallies(members.begin(), members.end());
    std::sort(tallies.begin(), tallies.end(), [](const MemberTally &a, const MemberTally &b) {
        const int totalA = a.primary + a.support;
        const int totalB = b.primary + b.support;
        return totalA != totalB ? totalA > totalB : a.name.localeAwareCompare(b.name) < 0;
    });
    html += QStringLiteral("<h2>Who served</h2><table width='100%'><tr><th>Member</th><th>Serving</th><th>Support</th><th>Total</th></tr>");
    for (const MemberTally &tally : tallies) {
        html += QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td><td><b>%4</b></td></tr>")
            .arg(esc(tally.name)).arg(tally.primary).arg(tally.support).arg(tally.primary + tally.support);
    }
    html += QStringLiteral("</table>");

    QVector<QPair<QString, int>> roleCounts;
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        roleCounts.append({it.key(), it.value()});
    }
    std::sort(roleCounts.begin(), roleCounts.end(), [](const auto &a, const auto &b) {
        return a.second != b.second ? a.second > b.second : a.first < b.first;
    });
    html += QStringLiteral("<h2>Roles</h2><table width='100%'><tr><th>Role</th><th>Times scheduled</th></tr>");
    for (const auto &role : roleCounts) {
        html += QStringLiteral("<tr><td>%1</td><td>%2</td></tr>").arg(esc(role.first)).arg(role.second);
    }
    html += QStringLiteral("</table>");
    return html;
}

void ReportsView::saveClicked()
{
    const QDate sunday = selectedSunday();
    const QString baseName = sunday.isValid()
        ? QStringLiteral("schedule-%1").arg(sunday.toString(Qt::ISODate))
        : QStringLiteral("schedule-summary-%1-to-%2")
              .arg(qMin(m_fromEdit->date(), m_toEdit->date()).toString(Qt::ISODate),
                   qMax(m_fromEdit->date(), m_toEdit->date()).toString(Qt::ISODate));
    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + QLatin1Char('/') + baseName + QStringLiteral(".html");
    QString selectedFilter;
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Save Report"), defaultPath,
        QStringLiteral("Web page (*.html);;Spreadsheet (*.csv)"), &selectedFilter);
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("Save Report"), file.errorString());
        return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    if (path.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive) || selectedFilter.contains(QLatin1String("csv"))) {
        // One row per assignment, whichever view is showing, so the file
        // works as-is in a spreadsheet.
        out << "Date,Role,Serving,Support,Notes,Status\n";
        for (const ScheduleReportRow &row : visibleRows()) {
            out << csvField(row.serviceDate.toString(Qt::ISODate)) << ','
                << csvField(row.roleName) << ','
                << csvField(row.memberName.isEmpty() ? QStringLiteral("Unfilled") : row.memberName) << ','
                << csvField(row.supportMemberName) << ','
                << csvField(row.notes) << ','
                << csvField(statusText(row)) << '\n';
        }
    } else {
        const QString body = sunday.isValid() ? sundayHtml(sunday) : summaryHtml();
        out << wrapHtml(sunday.isValid() ? formatSunday(sunday) : QStringLiteral("Schedule summary"), body);
    }
    file.close();
    m_statusLabel->setText(QStringLiteral("Saved to %1").arg(QDir::toNativeSeparators(path)));
}
