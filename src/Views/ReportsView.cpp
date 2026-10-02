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
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>

#include "Controllers/DutyController.h"
#include "Controllers/UserController.h"
#include "Style.h"

namespace
{
    // Marks the "All Sundays in range" entry in the Sunday list (Sunday rows store
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

    // Who did what: one row per duty -- duty, who served, backup, notes.
    QString lineupTable(const QVector<ScheduleReportRow> &rows, const ReportColors &c)
    {
        QString html = QStringLiteral("<table width='100%' cellspacing='0'>"
                                      "<tr><th class='cell'>Duty</th><th class='cell'>Serving</th>"
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

ReportsView::ReportsView(DutyController *dutyController, UserController *userController, QWidget *parent)
    : QWidget(parent)
    , m_dutyController(dutyController)
    , m_userController(userController)
{
    auto *title = new QLabel(QStringLiteral("Reports"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("Look up past Sunday schedules and who did what. Pick a date range, then one Sunday, "
                        "or \"All Sundays in range\" to see every Sunday in it."),
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
    // Tables are sized to the view's width; cell padding can push them a
    // few pixels past it, which isn't worth a scroll bar.
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
    m_rows = m_dutyController->scheduleReport(from, to, m_memberCombo->currentData().toInt());
    populateSundayList();
}

void ReportsView::populateSundayList()
{
    const QDate previouslySelected = selectedSunday();

    m_sundayList->blockSignals(true);
    m_sundayList->clear();
    auto *summary = new QListWidgetItem(QStringLiteral("All Sundays in range"), m_sundayList);
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
                .arg(count == 1 ? QStringLiteral("duty") : QStringLiteral("duties")),
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
        return wrapHtml(QStringLiteral("Who did what"), filterDescription(), rangeHtml(darkColors), darkColors);
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
    QString html;
    if (date > QDate::currentDate()) {
        html += QStringLiteral("<p style='color:%1;'>This Sunday is still ahead, so this is the current plan.</p>").arg(c.warn);
    }
    return html + lineupTable(visibleRows(), c);
}

QString ReportsView::rangeHtml(bool darkColors) const
{
    const ReportColors c = reportColors(darkColors);
    if (m_rows.isEmpty()) {
        return QStringLiteral("<p style='color:%1;'>No Sunday schedules in this date range. "
                              "Try a wider range with the buttons above.</p>").arg(c.muted);
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
        // One row per duty, whichever view is showing, so the file
        // works as-is in a spreadsheet.
        out << "Date,Duty,Serving,Backup,Notes,Availability\n";
        for (const ScheduleReportRow &row : visibleRows()) {
            out << csvField(row.serviceDate.toString(Qt::ISODate)) << ','
                << csvField(row.dutyTypeName) << ','
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
