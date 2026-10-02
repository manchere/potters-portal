#include "ReportsView.h"

#include <QLocale>
#include <QCoreApplication>
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
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>

#include "Controllers/DutyController.h"
#include "Controllers/UserController.h"
#include "ActionBar.h"
#include "MemberPickerField.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/TeamController.h"
#include "Style.h"
#include "MessageDialog.h"

namespace
{
    // The one filter field holds members, duty types and teams together;
    // each pick's id is kind * kKindStride + the record's own id.
    constexpr int kKindStride = 100000000;
    enum FilterKind { MemberFilter = 0, DutyFilter = 1, TeamFilter = 2 };

    // Marks the "All Sundays in range" entry in the Sunday list (Sunday rows store
    // their QDate instead).
    const QString kSummaryKey = QStringLiteral("summary");

    QString formatSunday(const QDate &date)
    {
        // Same format as the Schedule tab, e.g. "Sun 6 Sep 2026".
        return QLocale().toString(date, QStringLiteral("ddd d MMM yyyy"));
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
        QString warn;
        QString warnBg;
    };

    ReportColors reportColors(bool dark)
    {
        if (dark) {
            return {QStringLiteral("#e6e6e6"), QStringLiteral("#9a9a9a"), QStringLiteral("#262626"),
                    QStringLiteral("#e0b85a"), QStringLiteral("#2a2210")};
        }
        return {QStringLiteral("#1f2430"), QStringLiteral("#6b7280"), QStringLiteral("#e7e9f0"),
                QStringLiteral("#8a6a1a"), QStringLiteral("#faf3e0")};
    }

    // Shown next to the serving member's name only when their availability
    // matters for that Sunday; empty for everyone else, which is most
    // rows. A denied time-off request isn't flagged -- they were still
    // expected to serve.
    QString availabilityNote(const ScheduleReportRow &row)
    {
        if (row.requestStatus == QLatin1String("approved")) {
            return QCoreApplication::translate("ReportsView", "Time off approved");
        }
        if (row.requestStatus == QLatin1String("pending")) {
            return QCoreApplication::translate("ReportsView", "Asked for time off");
        }
        if (row.memberMarkedUnavailable) {
            return QCoreApplication::translate("ReportsView", "Marked away");
        }
        return QString();
    }

    QString tag(const QString &text, const ReportColors &c)
    {
        return QStringLiteral("&nbsp;&nbsp;<span style='background-color:%1; color:%2; font-size:8pt; font-weight:600;'>"
                              "&nbsp;%3&nbsp;</span>")
            .arg(c.warnBg, c.warn, esc(text));
    }

    // Who did what: one row per duty -- duty, who served, backup, notes.
    QString lineupTable(const QVector<ScheduleReportRow> &rows, const ReportColors &c)
    {
        QString html = QStringLiteral("<table width='100%' cellspacing='0'>"
                                      "<tr><th class='cell'>%1</th><th class='cell'>%2</th>"
                                      "<th class='cell'>%3</th><th class='cell'>%4</th></tr>")
            .arg(QCoreApplication::translate("ReportsView", "Duty"), QCoreApplication::translate("ReportsView", "Serving"),
                 QCoreApplication::translate("ReportsView", "Backup"), QCoreApplication::translate("ReportsView", "Notes"));
        for (const ScheduleReportRow &row : rows) {
            QString servingCell;
            if (row.memberId > 0) {
                servingCell = esc(row.memberName);
                const QString note = availabilityNote(row);
                if (!note.isEmpty()) {
                    servingCell += tag(note, c);
                }
            } else {
                servingCell = QStringLiteral("<span style='color:%1; font-weight:600;'>%2</span>")
                    .arg(c.warn, esc(QCoreApplication::translate("ReportsView", "Nobody assigned")));
            }
            const QString backupCell = row.supportMemberId > 0
                ? esc(row.supportMemberName)
                : QStringLiteral("<span style='color:%1;'>&mdash;</span>").arg(c.muted);
            html += QStringLiteral("<tr><td class='cell'><span>%1</span>&nbsp; <b>%2</b></td>"
                                   "<td class='cell'>%3</td><td class='cell'>%4</td>"
                                   "<td class='cell'><span style='color:%5;'>%6</span></td></tr>")
                .arg(esc(row.dutyTypeIcon), esc(row.dutyTypeName), servingCell, backupCell, c.muted, esc(row.notes));
        }
        return html + QStringLiteral("</table>");
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

ReportsView::ReportsView(DutyController *dutyController, UserController *userController,
                         DutyTypeController *dutyTypeController, TeamController *teamController,
                         QWidget *parent)
    : QWidget(parent)
    , m_dutyController(dutyController)
    , m_userController(userController)
    , m_dutyTypeController(dutyTypeController)
    , m_teamController(teamController)
{
    auto *title = new QLabel(tr("Reports"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        tr("Look up past Sunday schedules and who did what. Pick a date range, then one Sunday, "
                        "or \"All Sundays in range\" to see every Sunday in it."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    const QDate today = QDate::currentDate();
    m_fromEdit = new QDateEdit(today.addMonths(-3), this);
    m_toEdit = new QDateEdit(today, this);
    for (QDateEdit *edit : {m_fromEdit, m_toEdit}) {
        edit->setCalendarPopup(true);
        edit->setDisplayFormat(QStringLiteral("d MMM yyyy"));
        connect(edit, &QDateEdit::dateChanged, this, &ReportsView::reloadReport);
    }

    m_filterField = new MemberPickerField(this);
    m_filterField->setPlaceholders(tr("Everyone — type a member, duty or team to filter"), tr("Add another..."));
    m_filterField->setToolTip(tr("Members and teams together pick the people; duties narrow it to those duties."));
    connect(m_filterField, &MemberPickerField::selectionChanged, this, &ReportsView::reloadReport);

    auto *filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel(tr("From"), this));
    filterRow->addWidget(m_fromEdit);
    filterRow->addWidget(new QLabel(tr("To"), this));
    filterRow->addWidget(m_toEdit);
    filterRow->addStretch();

    // Its own row: the field grows as Members are added.
    auto *memberRow = new QHBoxLayout;
    auto *memberLabel = new QLabel(tr("Filter"), this);
    memberRow->addWidget(memberLabel, 0, Qt::AlignTop);
    memberRow->addWidget(m_filterField, 1);
    memberLabel->setMinimumHeight(m_fromEdit->sizeHint().height());

    m_sundayList = new QListWidget(this);
    m_sundayList->setFixedWidth(230);
    connect(m_sundayList, &QListWidget::currentRowChanged, this, &ReportsView::selectionChanged);
    auto *sundayBox = new QGroupBox(tr("Sundays"), this);
    auto *sundayLayout = new QVBoxLayout(sundayBox);
    sundayLayout->addWidget(m_sundayList);

    m_reportView = new QTextBrowser(this);
    m_reportView->setOpenLinks(false);
    // Tables are sized to the view's width; cell padding can push them a
    // few pixels past it, which isn't worth a scroll bar.
    m_reportView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_saveButton = new QPushButton(tr("Save Report..."), this);
    m_saveButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_saveButton, &QPushButton::clicked, this, &ReportsView::saveClicked);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("mutedLabel"));
    auto *reportBox = new QGroupBox(tr("Report"), this);
    auto *reportLayout = new QVBoxLayout(reportBox);
    reportLayout->addWidget(m_reportView, 1);
    reportLayout->addWidget(m_statusLabel);

    // Quick date ranges sit in the action bar too, under their own heading.
    auto *actionBar = new ActionBar(this);
    actionBar->addWidget(m_saveButton);
    actionBar->addSeparator();
    auto *rangeHeading = new QLabel(tr("Quick range"), this);
    rangeHeading->setObjectName(QStringLiteral("mutedLabel"));
    actionBar->addWidget(rangeHeading);
    const QList<QPair<QString, int>> presets = {
        {tr("Last month"), 1},
        {tr("Last 3 months"), 3},
        {tr("Last year"), 12},
    };
    for (const auto &preset : presets) {
        auto *button = new QPushButton(preset.first, this);
        button->setObjectName(QStringLiteral("secondaryButton"));
        const int months = preset.second;
        connect(button, &QPushButton::clicked, this, [this, months]() { presetClicked(months); });
        actionBar->addWidget(button);
    }
    actionBar->addStretch();

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(sundayBox);
    columns->addWidget(reportBox, 1);

    auto *content = new QVBoxLayout;
    content->setSpacing(12);
    content->addWidget(title);
    content->addWidget(subtitle);
    content->addSpacing(6);
    content->addLayout(filterRow);
    content->addLayout(memberRow);
    content->addLayout(columns, 1);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    layout->addLayout(content, 1);
    layout->addWidget(actionBar);

    refresh();
}

void ReportsView::refresh()
{
    populateFilters();
    reloadReport();
}

void ReportsView::populateFilters()
{
    // Each shows what it is: a person, the duty's own icon, or a group.
    QList<QPair<int, QString>> choices;
    for (const User &user : m_userController->allUsers()) {
        choices.append({MemberFilter * kKindStride + user.id(), QStringLiteral("\U0001F464 ") + user.name()});
    }
    for (const DutyType &dutyType : m_dutyTypeController->allDutyTypes()) {
        choices.append({DutyFilter * kKindStride + dutyType.id(), dutyType.iconAndName()});
    }
    for (const Team &team : m_teamController->allTeams()) {
        choices.append({TeamFilter * kKindStride + team.id(), QStringLiteral("\U0001F465 ") + team.name()});
    }
    // refresh() reloads the report right after, so no signal is needed.
    m_filterField->blockSignals(true);
    m_filterField->setMembers(choices);
    m_filterField->blockSignals(false);
}

void ReportsView::selectedFilters(QVector<int> *memberIds, QVector<int> *dutyTypeIds, QVector<int> *teamIds) const
{
    for (int key : m_filterField->selectedIds()) {
        const int id = key % kKindStride;
        switch (key / kKindStride) {
        case MemberFilter: memberIds->append(id); break;
        case DutyFilter: dutyTypeIds->append(id); break;
        case TeamFilter: teamIds->append(id); break;
        }
    }
}

QString ReportsView::selectionDescription() const
{
    return m_filterField->selectedNames().join(QStringLiteral(", "));
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
    QVector<int> memberIds;
    QVector<int> dutyTypeIds;
    QVector<int> teamIds;
    selectedFilters(&memberIds, &dutyTypeIds, &teamIds);
    m_rows = m_dutyController->scheduleReport(from, to, memberIds, dutyTypeIds, teamIds);
    populateSundayList();
}

void ReportsView::populateSundayList()
{
    const QDate previouslySelected = selectedSunday();

    m_sundayList->blockSignals(true);
    m_sundayList->clear();
    auto *summary = new QListWidgetItem(tr("All Sundays in range"), m_sundayList);
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
                .arg(count == 1 ? tr("duty") : tr("duties")),
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
    QString text = QStringLiteral("%1 – %2").arg(QLocale().toString(from, QStringLiteral("d MMM yyyy")), QLocale().toString(to, QStringLiteral("d MMM yyyy")));
    const QString selection = selectionDescription();
    if (!selection.isEmpty()) {
        text += QStringLiteral("  ·  %1").arg(selection);
    }
    return text;
}

void ReportsView::selectionChanged()
{
    m_reportView->setHtml(currentReportHtml(isDarkTheme(currentTheme())));

    const int sundays = m_sundayList->count() - 1;
    m_statusLabel->setText(sundays == 0
        ? tr("No schedules in this range.")
        : tr("%1 Sunday(s) with a schedule in this range.").arg(sundays));
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
        return wrapHtml(tr("Sunday Serving Report"), filterDescription(), rangeHtml(darkColors), darkColors);
    }
    const QString selection = selectionDescription();
    const QString subtitle = !selection.isEmpty()
        ? tr("Only showing %1").arg(selection)
        : tr("Sunday line-up");
    return wrapHtml(formatSunday(sunday), subtitle, sundayHtml(sunday, darkColors), darkColors);
}

QString ReportsView::wrapHtml(const QString &title, const QString &subtitle, const QString &body, bool darkColors) const
{
    const ReportColors c = reportColors(darkColors);
    // Sizes match the Schedule tab (Style.cpp): 10pt text, bold 10pt
    // names, 9pt column captions, the page title's 15pt for the heading.
    // Kept to the subset of CSS QTextBrowser understands, so the saved
    // file and the on-screen view look alike. Row lines are set per cell
    // (class "cell") so they don't leak into the tile and bar tables.
    // Colors are filled in before the body is appended, so text in the
    // body (e.g. a note containing "%1") is never treated as a placeholder.
    const QString head = QStringLiteral(
        "<html><head><meta charset='utf-8'><title>%1</title>"
        "<style>"
        "body { font-family: 'Segoe UI', sans-serif; font-size: 10pt; color: %2; }"
        "h1 { font-size: 15pt; margin-bottom: 0px; color: %2; }"
        "h2 { font-size: 11pt; margin-top: 18px; margin-bottom: 4px; color: %2; }"
        "th.cell { text-align: left; font-size: 9pt; font-weight: 600; color: %3; padding: 6px 10px; border-bottom: 1px solid %4; }"
        "td.cell { padding: 7px 10px; border-bottom: 1px solid %4; vertical-align: middle; }"
        "</style></head><body>"
        "<h1>%1</h1><p style='color:%3; margin-top:2px;'>%5</p>")
        .arg(esc(title), c.ink, c.muted, c.line, esc(subtitle));
    return head + body + QStringLiteral("</body></html>");
}

QString ReportsView::sundayHtml(const QDate &date, bool darkColors) const
{
    const ReportColors c = reportColors(darkColors);
    QString html;
    if (date > QDate::currentDate()) {
        html += tr("<p style='color:%1;'>This Sunday is still ahead, so this is the current plan.</p>").arg(c.warn);
    }
    return html + lineupTable(visibleRows(), c);
}

QString ReportsView::rangeHtml(bool darkColors) const
{
    const ReportColors c = reportColors(darkColors);
    if (m_rows.isEmpty()) {
        return tr("<p style='color:%1;'>No Sunday schedules in this date range. "
                              "Try a wider range from Quick range on the right.</p>").arg(c.muted);
    }

    // m_rows is newest first; keep that order, and each Sunday's duty order.
    QVector<QDate> dates;
    QMap<QDate, QVector<ScheduleReportRow>> rowsByDate;
    for (const ScheduleReportRow &row : m_rows) {
        if (!rowsByDate.contains(row.serviceDate)) {
            dates.append(row.serviceDate);
        }
        rowsByDate[row.serviceDate].append(row);
    }
    QString html;
    for (const QDate &date : dates) {
        html += QStringLiteral("<h2>%1</h2>").arg(esc(formatSunday(date)));
        html += lineupTable(rowsByDate.value(date), c);
    }
    return html;
}

void ReportsView::setCanSave(bool canSave)
{
    m_canSave = canSave;
    m_saveButton->setVisible(canSave);
}

void ReportsView::saveClicked()
{
    if (!m_canSave) {
        return;
    }
    const QDate sunday = selectedSunday();
    const QString baseName = sunday.isValid()
        ? QStringLiteral("schedule-%1").arg(sunday.toString(Qt::ISODate))
        : QStringLiteral("schedule-summary-%1-to-%2")
              .arg(qMin(m_fromEdit->date(), m_toEdit->date()).toString(Qt::ISODate),
                   qMax(m_fromEdit->date(), m_toEdit->date()).toString(Qt::ISODate));
    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + QLatin1Char('/') + baseName + QStringLiteral(".html");
    QString selectedFilter;
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Report"), defaultPath,
        tr("Web page (*.html);;Spreadsheet (*.csv)"), &selectedFilter);
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        MessageDialog::critical(this, tr("Save Report"), file.errorString());
        return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    if (path.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive) || selectedFilter.contains(QLatin1String("csv"))) {
        // One row per duty, whichever view is showing, so the file
        // works as-is in a spreadsheet.
        out << "Date,Duty,Serving,Backup,Notes,Availability\n";
        for (const ScheduleReportRow &row : visibleRows()) {
            out << csvField(row.serviceDate.toString(QStringLiteral("dd/MM/yyyy"))) << ','
                << csvField(row.dutyTypeName) << ','
                << csvField(row.memberName.isEmpty() ? tr("Unfilled") : row.memberName) << ','
                << csvField(row.supportMemberName) << ','
                << csvField(row.notes) << ','
                << csvField(availabilityNote(row)) << '\n';
        }
    } else {
        out << currentReportHtml(false);
    }
    file.close();
    m_statusLabel->setText(tr("Saved to %1").arg(QDir::toNativeSeparators(path)));
}
