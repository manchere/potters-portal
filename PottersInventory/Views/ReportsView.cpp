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
#include "Style.h"

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

    // The report's colors. QTextBrowser can't read the app stylesheet, so
    // they're spelled out here: the light set matches the app's navy/gold
    // brand, the dark set matches Style.cpp's black theme.
    struct ReportColors
    {
        QString ink;
        QString muted;
        QString line;
        QString tile;
        QString accent;
        QString track;
        QString warn;
        QString warnBg;
    };

    ReportColors reportColors(bool dark)
    {
        if (dark) {
            return {QStringLiteral("#e6e6e6"), QStringLiteral("#9a9a9a"), QStringLiteral("#262626"),
                    QStringLiteral("#161616"), QStringLiteral("#6f9be0"), QStringLiteral("#1f1f1f"),
                    QStringLiteral("#e0b85a"), QStringLiteral("#2a2210")};
        }
        return {QStringLiteral("#1f2430"), QStringLiteral("#6b7280"), QStringLiteral("#e7e9f0"),
                QStringLiteral("#f2f4f8"), QStringLiteral("#14335c"), QStringLiteral("#e9edf5"),
                QStringLiteral("#8a6a1a"), QStringLiteral("#faf3e0")};
    }

    // Shown next to the serving member's name only when their availability
    // matters for that Sunday; empty for everyone else, which is most
    // rows. A denied time-off request isn't flagged -- they were still
    // expected to serve.
    QString availabilityNote(const ScheduleReportRow &row)
    {
        if (row.requestStatus == QLatin1String("approved")) {
            return QStringLiteral("Time off approved");
        }
        if (row.requestStatus == QLatin1String("pending")) {
            return QStringLiteral("Asked for time off");
        }
        if (row.memberMarkedUnavailable) {
            return QStringLiteral("Marked away");
        }
        return QString();
    }

    QString tag(const QString &text, const ReportColors &c)
    {
        return QStringLiteral("&nbsp;&nbsp;<span style='background-color:%1; color:%2; font-size:11px; font-weight:600;'>"
                              "&nbsp;%3&nbsp;</span>")
            .arg(c.warnBg, c.warn, esc(text));
    }

    // A row of big-number tiles, e.g. "4 roles | 3 people serving". The
    // tile at warnIndex turns gold when its number is above zero.
    QString tiles(const QList<QPair<int, QString>> &items, const ReportColors &c, int warnIndex = -1)
    {
        QString html = QStringLiteral("<table width='100%' cellspacing='8' cellpadding='12'><tr>");
        for (int i = 0; i < items.size(); ++i) {
            const bool warn = i == warnIndex && items[i].first > 0;
            html += QStringLiteral("<td bgcolor='%1' width='%2%'>"
                                   "<span style='font-size:24px; font-weight:700; color:%3;'>%4</span><br>"
                                   "<span style='color:%5;'>%6</span></td>")
                .arg(warn ? c.warnBg : c.tile)
                .arg(100 / items.size())
                .arg(warn ? c.warn : c.ink)
                .arg(items[i].first)
                .arg(warn ? c.warn : c.muted, esc(items[i].second));
        }
        return html + QStringLiteral("</tr></table>");
    }

    // A horizontal bar scaled against max, drawn as a two-cell table since
    // QTextBrowser can't size inline elements.
    QString bar(int value, int max, const ReportColors &c)
    {
        const int percent = max > 0 ? qBound(3, value * 100 / max, 100) : 0;
        QString html = QStringLiteral("<table width='100%' cellspacing='0' cellpadding='0'><tr>"
                                      "<td width='%1%' bgcolor='%2'><span style='font-size:8px;'>&nbsp;</span></td>")
            .arg(percent).arg(c.accent);
        if (percent < 100) {
            html += QStringLiteral("<td bgcolor='%1'><span style='font-size:8px;'>&nbsp;</span></td>").arg(c.track);
        }
        return html + QStringLiteral("</tr></table>");
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
    // Tables are sized to the view's width; the tiles' cell spacing can
    // push them a few pixels past it, which isn't worth a scroll bar.
    m_reportView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_saveButton = new QPushButton(QStringLiteral("Save Report..."), this);
    m_saveButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_saveButton, &QPushButton::clicked, this, &ReportsView::saveClicked);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("mutedLabel"));
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
    m_reportView->setHtml(currentReportHtml(currentTheme() == Theme::Black));

    const int sundays = m_sundayList->count() - 1;
    m_statusLabel->setText(sundays == 0
        ? QStringLiteral("No schedules in this range.")
        : QStringLiteral("%1 Sunday(s) with a schedule in this range.").arg(sundays));
    m_saveButton->setEnabled(!m_rows.isEmpty());
}

void ReportsView::restyleReport()
{
    selectionChanged();
}

QString ReportsView::currentReportHtml(bool darkColors) const
{
    const QDate sunday = selectedSunday();
    if (!sunday.isValid()) {
        return wrapHtml(QStringLiteral("Schedule summary"), filterDescription(), summaryHtml(darkColors), darkColors);
    }
    const QString subtitle = m_memberCombo->currentData().toInt() > 0
        ? QStringLiteral("Only showing %1").arg(m_memberCombo->currentText())
        : QStringLiteral("Sunday line-up");
    return wrapHtml(formatSunday(sunday), subtitle, sundayHtml(sunday, darkColors), darkColors);
}

QString ReportsView::wrapHtml(const QString &title, const QString &subtitle, const QString &body, bool darkColors) const
{
    const ReportColors c = reportColors(darkColors);
    // Kept to the subset of CSS QTextBrowser understands, so the saved
    // file and the on-screen view look alike. Row lines are set per cell
    // (class "cell") so they don't leak into the tile and bar tables.
    // Colors are filled in before the body is appended, so text in the
    // body (e.g. a note containing "%1") is never treated as a placeholder.
    const QString head = QStringLiteral(
        "<html><head><meta charset='utf-8'><title>%1</title>"
        "<style>"
        "body { font-family: 'Segoe UI', sans-serif; font-size: 14px; color: %2; }"
        "h1 { font-size: 22px; margin-bottom: 0px; color: %2; }"
        "h2 { font-size: 15px; margin-top: 22px; margin-bottom: 4px; color: %2; }"
        "th.cell { text-align: left; font-weight: 600; color: %3; padding: 8px 10px; border-bottom: 1px solid %4; }"
        "td.cell { padding: 9px 10px; border-bottom: 1px solid %4; vertical-align: middle; }"
        "</style></head><body>"
        "<h1>%1</h1><p style='color:%3; margin-top:2px;'>%5</p>")
        .arg(esc(title), c.ink, c.muted, c.line, esc(subtitle));
    return head + body + QStringLiteral("</body></html>");
}

QString ReportsView::sundayHtml(const QDate &date, bool darkColors) const
{
    const ReportColors c = reportColors(darkColors);
    const QVector<ScheduleReportRow> rows = visibleRows();
    QSet<int> serving;
    int unfilled = 0;
    int away = 0;
    for (const ScheduleReportRow &row : rows) {
        if (row.memberId > 0) {
            serving.insert(row.memberId);
            if (!availabilityNote(row).isEmpty()) {
                ++away;
            }
        } else {
            ++unfilled;
        }
        if (row.supportMemberId > 0) {
            serving.insert(row.supportMemberId);
        }
    }

    QList<QPair<int, QString>> totals = {
        {int(rows.size()), rows.size() == 1 ? QStringLiteral("role") : QStringLiteral("roles")},
        {int(serving.size()), QStringLiteral("people serving")},
        {unfilled, QStringLiteral("not filled")},
    };
    QString html = tiles(totals, c, 2);
    if (date > QDate::currentDate()) {
        html += QStringLiteral("<p style='color:%1;'>This Sunday is still ahead, so this is the current plan.</p>").arg(c.warn);
    }
    if (away > 0) {
        html += QStringLiteral("<p style='color:%1;'>%2 of the people serving had asked for time off or marked "
                               "themselves away. They're tagged below.</p>").arg(c.warn).arg(away);
    }

    html += QStringLiteral("<h2>Line-up</h2><table width='100%' cellspacing='0'>"
                           "<tr><th class='cell'>Role</th><th class='cell'>Serving</th>"
                           "<th class='cell'>Backup</th><th class='cell'>Notes</th></tr>");
    for (const ScheduleReportRow &row : rows) {
        QString servingCell;
        if (row.memberId > 0) {
            servingCell = esc(row.memberName);
            const QString note = availabilityNote(row);
            if (!note.isEmpty()) {
                servingCell += tag(note, c);
            }
        } else {
            servingCell = QStringLiteral("<span style='color:%1; font-weight:600;'>Nobody assigned</span>").arg(c.warn);
        }
        const QString backupCell = row.supportMemberId > 0
            ? esc(row.supportMemberName)
            : QStringLiteral("<span style='color:%1;'>&mdash;</span>").arg(c.muted);
        html += QStringLiteral("<tr><td class='cell'><span style='font-size:16px;'>%1</span>&nbsp; <b>%2</b></td>"
                               "<td class='cell'>%3</td><td class='cell'>%4</td>"
                               "<td class='cell'><span style='color:%5;'>%6</span></td></tr>")
            .arg(esc(row.roleIcon), esc(row.roleName), servingCell, backupCell, c.muted, esc(row.notes));
    }
    html += QStringLiteral("</table>");
    return html;
}

QString ReportsView::summaryHtml(bool darkColors) const
{
    const ReportColors c = reportColors(darkColors);
    if (m_rows.isEmpty()) {
        return QStringLiteral("<p style='color:%1;'>No Sunday schedules in this date range. "
                              "Try a wider range with the buttons above.</p>").arg(c.muted);
    }

    struct MemberTally { QString name; int primary = 0; int support = 0; };
    struct SundayTally { int roles = 0; int filled = 0; QStringList unfilledRoles; int away = 0; };
    QMap<int, MemberTally> members;
    QMap<QString, int> roles;
    QMap<QDate, SundayTally> sundays;
    int unfilled = 0;
    int timeOff = 0;
    for (const ScheduleReportRow &row : m_rows) {
        SundayTally &sunday = sundays[row.serviceDate];
        ++sunday.roles;
        ++roles[row.roleIcon + QStringLiteral("  ") + row.roleName];
        if (row.memberId > 0) {
            ++sunday.filled;
            members[row.memberId].name = row.memberName;
            ++members[row.memberId].primary;
            if (!availabilityNote(row).isEmpty()) {
                ++sunday.away;
                ++timeOff;
            }
        } else {
            ++unfilled;
            sunday.unfilledRoles << row.roleName;
        }
        if (row.supportMemberId > 0) {
            members[row.supportMemberId].name = row.supportMemberName;
            ++members[row.supportMemberId].support;
        }
    }

    QList<QPair<int, QString>> totals = {
        {int(sundays.size()), sundays.size() == 1 ? QStringLiteral("Sunday") : QStringLiteral("Sundays")},
        {int(m_rows.size()), QStringLiteral("roles scheduled")},
        {int(members.size()), QStringLiteral("people serving")},
        {unfilled, QStringLiteral("not filled")},
    };
    QString html = tiles(totals, c, 3);
    if (timeOff > 0) {
        html += QStringLiteral("<p style='color:%1;'>People asked for time off or marked themselves away "
                               "%2 time(s) when they were scheduled. See Sunday by Sunday below.</p>").arg(c.warn).arg(timeOff);
    }

    // Who served: most first, with a bar against the busiest person.
    QVector<MemberTally> tallies(members.begin(), members.end());
    std::sort(tallies.begin(), tallies.end(), [](const MemberTally &a, const MemberTally &b) {
        const int totalA = a.primary + a.support;
        const int totalB = b.primary + b.support;
        return totalA != totalB ? totalA > totalB : a.name.localeAwareCompare(b.name) < 0;
    });
    const int maxServed = tallies.isEmpty() ? 0 : tallies.first().primary + tallies.first().support;
    html += QStringLiteral("<h2>Who served</h2><table width='100%' cellspacing='0'>");
    for (const MemberTally &tally : tallies) {
        const int total = tally.primary + tally.support;
        const QString detail = tally.support > 0
            ? QStringLiteral("%1 <span style='color:%2;'>(%3 as backup)</span>").arg(total).arg(c.muted).arg(tally.support)
            : QString::number(total);
        html += QStringLiteral("<tr><td class='cell' width='28%'>%1</td><td class='cell' width='52%'>%2</td>"
                               "<td class='cell'>%3</td></tr>")
            .arg(esc(tally.name), bar(total, maxServed, c), detail);
    }
    html += QStringLiteral("</table>");

    QVector<QPair<QString, int>> roleCounts;
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        roleCounts.append({it.key(), it.value()});
    }
    std::sort(roleCounts.begin(), roleCounts.end(), [](const auto &a, const auto &b) {
        return a.second != b.second ? a.second > b.second : a.first < b.first;
    });
    const int maxRole = roleCounts.isEmpty() ? 0 : roleCounts.first().second;
    html += QStringLiteral("<h2>Roles</h2><table width='100%' cellspacing='0'>");
    for (const auto &role : roleCounts) {
        html += QStringLiteral("<tr><td class='cell' width='28%'>%1</td><td class='cell' width='52%'>%2</td>"
                               "<td class='cell'>%3 time(s)</td></tr>")
            .arg(esc(role.first), bar(role.second, maxRole, c)).arg(role.second);
    }
    html += QStringLiteral("</table>");

    // Sunday by Sunday, newest first.
    html += QStringLiteral("<h2>Sunday by Sunday</h2><table width='100%' cellspacing='0'>"
                           "<tr><th class='cell'>Sunday</th><th class='cell'>Roles filled</th>"
                           "<th class='cell'>Needs attention</th></tr>");
    for (auto it = sundays.cend(); it != sundays.cbegin();) {
        --it;
        const SundayTally &sunday = it.value();
        QStringList attention;
        if (!sunday.unfilledRoles.isEmpty()) {
            attention << QStringLiteral("Not filled: %1").arg(sunday.unfilledRoles.join(QStringLiteral(", ")));
        }
        if (sunday.away > 0) {
            attention << QStringLiteral("%1 away").arg(sunday.away);
        }
        html += QStringLiteral("<tr><td class='cell'>%1</td><td class='cell'>%2 of %3</td>"
                               "<td class='cell'><span style='color:%4;'>%5</span></td></tr>")
            .arg(esc(formatSunday(it.key())))
            .arg(sunday.filled)
            .arg(sunday.roles)
            .arg(attention.isEmpty() ? c.muted : c.warn,
                 attention.isEmpty() ? QStringLiteral("&mdash;") : esc(attention.join(QStringLiteral(" · "))));
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
        out << "Date,Role,Serving,Backup,Notes,Availability\n";
        for (const ScheduleReportRow &row : visibleRows()) {
            out << csvField(row.serviceDate.toString(Qt::ISODate)) << ','
                << csvField(row.roleName) << ','
                << csvField(row.memberName.isEmpty() ? QStringLiteral("Unfilled") : row.memberName) << ','
                << csvField(row.supportMemberName) << ','
                << csvField(row.notes) << ','
                << csvField(availabilityNote(row)) << '\n';
        }
    } else {
        out << currentReportHtml(false);
    }
    file.close();
    m_statusLabel->setText(QStringLiteral("Saved to %1").arg(QDir::toNativeSeparators(path)));
}
