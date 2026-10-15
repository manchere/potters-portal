#include "ScheduleBoard.h"

#include <algorithm>

#include <QApplication>
#include <QDataStream>
#include <QDrag>
#include <QDragEnterEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollArea>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "ElidedLabel.h"
#include "MemberBadge.h"

namespace
{
    const QString kMimeType = QStringLiteral("application/x-pottersportal-board-member");

    constexpr int kCellWidth = 190;
    constexpr int kDateWidth = 130;

    // Deletes everything in layout. deleteLater, since this can run while
    // a drag that started in one of those widgets is still unwinding.
    void clearLayout(QLayout *layout)
    {
        while (QLayoutItem *item = layout->takeAt(0)) {
            if (QWidget *widget = item->widget()) {
                widget->hide();
                widget->deleteLater();
            }
            delete item;
        }
    }

    void repolish(QWidget *widget)
    {
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }

    QString columnKey(int dutyTypeId, const QString &name)
    {
        return dutyTypeId > 0 ? QStringLiteral("#%1").arg(dutyTypeId) : name;
    }
}

namespace ScheduleDrag
{
    QMimeData *encode(const Payload &payload)
    {
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out << qint32(payload.memberId) << qint32(payload.fromDutyId) << qint32(payload.fromSlot);
        auto *mime = new QMimeData;
        mime->setData(kMimeType, data);
        return mime;
    }

    bool decode(const QMimeData *mime, Payload *payload)
    {
        if (!mime || !mime->hasFormat(kMimeType)) {
            return false;
        }
        QDataStream in(mime->data(kMimeType));
        qint32 memberId = -1;
        qint32 fromDutyId = -1;
        qint32 fromSlot = NoSlot;
        in >> memberId >> fromDutyId >> fromSlot;
        if (in.status() != QDataStream::Ok || memberId <= 0) {
            return false;
        }
        payload->memberId = memberId;
        payload->fromDutyId = fromDutyId;
        payload->fromSlot = fromSlot;
        return true;
    }
}

// --- BoardDropZone ----------------------------------------------------------

BoardDropZone::BoardDropZone(QWidget *parent)
    : QFrame(parent)
{
    setAcceptDrops(true);
}

void BoardDropZone::dragEnterEvent(QDragEnterEvent *event)
{
    ScheduleDrag::Payload payload;
    if (!onDrop || !ScheduleDrag::decode(event->mimeData(), &payload) || (accepts && !accepts(payload))) {
        event->ignore();
        return;
    }
    event->setDropAction(Qt::MoveAction);
    event->accept();
    setHover(true);
}

void BoardDropZone::dragLeaveEvent(QDragLeaveEvent *event)
{
    setHover(false);
    QFrame::dragLeaveEvent(event);
}

void BoardDropZone::dropEvent(QDropEvent *event)
{
    setHover(false);
    ScheduleDrag::Payload payload;
    if (!onDrop || !ScheduleDrag::decode(event->mimeData(), &payload) || (accepts && !accepts(payload))) {
        event->ignore();
        return;
    }
    event->setDropAction(Qt::MoveAction);
    event->accept();
    onDrop(payload);
}

void BoardDropZone::setHover(bool hover)
{
    if (property("dropHover").toBool() != hover) {
        setProperty("dropHover", hover);
        repolish(this);
    }
}

// --- BoardMemberChip --------------------------------------------------------

BoardMemberChip::BoardMemberChip(const QString &name, const QString &color, QWidget *parent, int badgeSize)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("boardChip"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 3, 3, 3);
    layout->setSpacing(6);
    layout->addWidget(MemberBadge::make(name, color, badgeSize, this), 0, Qt::AlignVCenter);

    m_textLayout = new QVBoxLayout;
    m_textLayout->setContentsMargins(0, 0, 0, 0);
    m_textLayout->setSpacing(0);
    m_nameRow = new QWidget(this);
    auto *nameLayout = new QHBoxLayout(m_nameRow);
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(6);
    auto *nameLabel = new ElidedLabel(name, m_nameRow);
    nameLabel->setObjectName(QStringLiteral("dutyMemberName"));
    nameLayout->addWidget(nameLabel, 1);
    m_textLayout->addWidget(m_nameRow);
    layout->addLayout(m_textLayout, 1);
}

void BoardMemberChip::setTag(const QString &tag, const QString &toolTip)
{
    if (tag.isEmpty()) {
        return;
    }
    auto *label = new QLabel(tag, m_nameRow);
    label->setObjectName(QStringLiteral("boardTag"));
    label->setToolTip(toolTip);
    m_nameRow->layout()->addWidget(label);
}

void BoardMemberChip::setDetail(const QString &detail)
{
    if (detail.isEmpty()) {
        return;
    }
    auto *label = new QLabel(detail, this);
    label->setObjectName(QStringLiteral("boardHint"));
    m_textLayout->addWidget(label);
}

void BoardMemberChip::setDraggable(const ScheduleDrag::Payload &payload)
{
    m_draggable = true;
    m_payload = payload;
    setCursor(Qt::OpenHandCursor);
}

void BoardMemberChip::setRemovable(const QString &toolTip, std::function<void()> onRemove)
{
    auto *remove = new QToolButton(this);
    remove->setObjectName(QStringLiteral("boardChipRemove"));
    remove->setText(QStringLiteral("×"));
    remove->setToolTip(toolTip);
    remove->setCursor(Qt::PointingHandCursor);
    connect(remove, &QToolButton::clicked, this, std::move(onRemove));
    layout()->addWidget(remove);
}

void BoardMemberChip::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressPos = event->position().toPoint();
    }
    QFrame::mousePressEvent(event);
}

void BoardMemberChip::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_draggable || !(event->buttons() & Qt::LeftButton)
        || (event->position().toPoint() - m_pressPos).manhattanLength() < QApplication::startDragDistance()) {
        QFrame::mouseMoveEvent(event);
        return;
    }
    auto *drag = new QDrag(this);
    drag->setMimeData(ScheduleDrag::encode(m_payload));
    drag->setPixmap(grab());
    drag->setHotSpot(m_pressPos);
    // The drop may rebuild the board this chip is on (deleteLater), so
    // nothing here touches the chip afterwards.
    drag->exec(Qt::MoveAction);
}

// --- ScheduleBoard ----------------------------------------------------------

ScheduleBoard::ScheduleBoard(QWidget *parent)
    : QWidget(parent)
{
    m_emptyLabel = new QLabel(tr("No duties to show."), this);
    m_emptyLabel->setObjectName(QStringLiteral("mutedLabel"));
    m_emptyLabel->hide();

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);
    m_layout->addWidget(m_emptyLabel);
    m_layout->addStretch();
}

void ScheduleBoard::setSchedule(const QVector<QDate> &dates, const QVector<DutyType> &columns,
                                const QVector<BoardDuty> &duties)
{
    m_dates = dates;
    m_columns = columns;
    m_duties = duties;
    rebuild();
}

void ScheduleBoard::setEditable(bool editable, const QDate &editableFrom)
{
    m_editable = editable;
    m_editableFrom = editableFrom;
}

void ScheduleBoard::setHighlightedDate(const QDate &date)
{
    m_highlightedDate = date;
}

bool ScheduleBoard::rowEditable(const QDate &date) const
{
    return m_editable && (!m_editableFrom.isValid() || date >= m_editableFrom);
}

void ScheduleBoard::rebuild()
{
    if (m_grid) {
        m_layout->removeWidget(m_grid);
        m_grid->hide();
        m_grid->deleteLater();
        m_grid = nullptr;
    }

    // Columns: the given duty types, else whichever the duties use, in
    // the order they first appear.
    struct Column
    {
        QString key;
        int dutyTypeId = -1;
        QString icon;
        QString name;
    };
    QVector<Column> columns;
    for (const DutyType &dutyType : std::as_const(m_columns)) {
        columns.append({columnKey(dutyType.id(), dutyType.name()), dutyType.id(), dutyType.icon(), dutyType.name()});
    }
    if (columns.isEmpty()) {
        for (const BoardDuty &duty : std::as_const(m_duties)) {
            const QString key = columnKey(duty.dutyTypeId, duty.dutyTypeName);
            if (std::none_of(columns.cbegin(), columns.cend(), [&key](const Column &c) { return c.key == key; })) {
                columns.append({key, duty.dutyTypeId, duty.dutyTypeIcon, duty.dutyTypeName});
            }
        }
    }

    m_emptyLabel->setVisible(m_dates.isEmpty() || columns.isEmpty());
    if (m_dates.isEmpty() || columns.isEmpty()) {
        return;
    }

    m_grid = new QWidget(this);
    m_grid->setObjectName(QStringLiteral("boardSurface"));
    auto *grid = new QGridLayout(m_grid);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(6);

    int row = 0;
    auto *dateHeader = new QLabel(tr("Date (Sunday)"), m_grid);
    dateHeader->setObjectName(QStringLiteral("boardColumnTitle"));
    dateHeader->setFixedWidth(kDateWidth);
    grid->addWidget(dateHeader, row, 0);
    for (int c = 0; c < columns.size(); ++c) {
        auto *header = new ElidedLabel(QStringLiteral("%1  %2").arg(columns[c].icon, columns[c].name), m_grid);
        header->setObjectName(QStringLiteral("boardColumnTitle"));
        header->setFixedWidth(kCellWidth);
        grid->addWidget(header, row, c + 1);
    }
    ++row;

    int month = -1;
    for (const QDate &date : std::as_const(m_dates)) {
        // A heading each time the month changes, like a rota sheet.
        const int thisMonth = date.year() * 12 + date.month();
        if (thisMonth != month) {
            month = thisMonth;
            auto *heading = new QLabel(QLocale().toString(date, QStringLiteral("MMMM yyyy")), m_grid);
            heading->setObjectName(QStringLiteral("boardMonthTitle"));
            grid->addWidget(heading, row++, 0, 1, columns.size() + 1);
        }

        auto *dateButton = new QToolButton(m_grid);
        dateButton->setObjectName(QStringLiteral("boardDateButton"));
        dateButton->setProperty("highlighted", date == m_highlightedDate);
        dateButton->setText(QLocale().toString(date, QStringLiteral("ddd d MMM")));
        dateButton->setToolTip(QLocale().toString(date, QStringLiteral("dddd d MMMM yyyy")));
        dateButton->setFixedWidth(kDateWidth);
        dateButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        dateButton->setCursor(Qt::PointingHandCursor);
        connect(dateButton, &QToolButton::clicked, this, [this, date] {
            QTimer::singleShot(0, this, [this, date] { emit dateActivated(date); });
        });
        grid->addWidget(dateButton, row, 0);

        for (int c = 0; c < columns.size(); ++c) {
            QVector<BoardDuty> cellDuties;
            for (const BoardDuty &duty : std::as_const(m_duties)) {
                if (duty.serviceDate == date && columnKey(duty.dutyTypeId, duty.dutyTypeName) == columns[c].key) {
                    cellDuties.append(duty);
                }
            }
            grid->addWidget(buildCell(date, columns[c].dutyTypeId, cellDuties, m_grid), row, c + 1);
        }
        ++row;
    }
    grid->setColumnStretch(columns.size() + 1, 1);
    m_layout->insertWidget(1, m_grid);
}

// One Sunday's duties of one type. In an editable row the whole cell takes
// drops (to put someone on that duty that Sunday).
QWidget *ScheduleBoard::buildCell(const QDate &date, int dutyTypeId, const QVector<BoardDuty> &duties, QWidget *parent)
{
    const bool editable = rowEditable(date) && dutyTypeId > 0;
    auto *cell = new BoardDropZone(parent);
    cell->setObjectName(QStringLiteral("boardCell"));
    cell->setProperty("highlighted", date == m_highlightedDate);
    cell->setFixedWidth(kCellWidth);
    auto *layout = new QVBoxLayout(cell);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    for (const BoardDuty &duty : duties) {
        layout->addWidget(buildDutyBlock(duty, editable, cell));
    }
    if (editable && duties.isEmpty()) {
        auto *hint = new QLabel(tr("Drop a member here"), cell);
        hint->setObjectName(QStringLiteral("boardHint"));
        hint->setAlignment(Qt::AlignCenter);
        hint->setMinimumHeight(30);
        layout->addWidget(hint);
    } else if (!editable && duties.isEmpty()) {
        auto *none = new QLabel(QStringLiteral("—"), cell);
        none->setObjectName(QStringLiteral("mutedLabel"));
        layout->addWidget(none);
    }
    layout->addStretch();

    if (editable) {
        // Moving someone who already serves this duty that Sunday onto it
        // changes nothing, so it isn't offered.
        const QVector<BoardDuty> here = duties;
        cell->accepts = [here](const ScheduleDrag::Payload &payload) {
            return payload.fromSlot != ScheduleDrag::Serving
                || std::none_of(here.cbegin(), here.cend(),
                                [&payload](const BoardDuty &duty) { return duty.dutyId == payload.fromDutyId; });
        };
        cell->onDrop = [this, date, dutyTypeId](const ScheduleDrag::Payload &payload) {
            QTimer::singleShot(0, this, [this, date, dutyTypeId, payload] {
                emit addRequested(date, dutyTypeId, payload.memberId, payload.fromDutyId, payload.fromSlot);
            });
        };
    }
    return cell;
}

// Who serves, their backup underneath (always, when editable, as a place to
// drop one), and any notes.
QWidget *ScheduleBoard::buildDutyBlock(const BoardDuty &duty, bool editable, QWidget *parent)
{
    auto *block = new QWidget(parent);
    auto *layout = new QVBoxLayout(block);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(buildServing(duty, editable, block));
    if (editable || duty.supportMemberId > 0) {
        layout->addWidget(buildBackup(duty, editable, block));
    }
    if (!duty.notes.isEmpty()) {
        auto *notes = new ElidedLabel(duty.notes, block);
        notes->setObjectName(QStringLiteral("boardHint"));
        layout->addWidget(notes);
    }
    return block;
}

QWidget *ScheduleBoard::buildServing(const BoardDuty &duty, bool editable, QWidget *parent)
{
    const bool filled = duty.memberId > 0 && !duty.memberName.isEmpty();
    QWidget *content = nullptr;
    BoardDropZone *zone = editable ? new BoardDropZone(parent) : nullptr;
    QWidget *owner = zone ? static_cast<QWidget *>(zone) : parent;
    if (filled) {
        auto *chip = new BoardMemberChip(duty.memberName, duty.memberColor, owner);
        chip->setTag(duty.memberTag, duty.memberTagToolTip);
        if (editable) {
            const int dutyId = duty.dutyId;
            chip->setDraggable({duty.memberId, dutyId, ScheduleDrag::Serving});
            chip->setRemovable(duty.supportMemberId > 0
                    ? tr("Take %1 off this duty; their backup, %2, serves instead").arg(duty.memberName, duty.supportMemberName)
                    : tr("Take %1 off this duty").arg(duty.memberName),
                [this, dutyId] {
                    QTimer::singleShot(0, this, [this, dutyId] { emit removeRequested(dutyId, ScheduleDrag::Serving); });
                });
            chip->setToolTip(tr("Drag to another cell or slot to move or swap. Drag back to the members list to take off."));
        }
        content = chip;
    } else {
        auto *nobody = new QLabel(tr("Nobody assigned"), owner);
        nobody->setObjectName(QStringLiteral("dutyUnfilled"));
        content = nobody;
    }
    if (!zone) {
        return content;
    }
    zone->setObjectName(QStringLiteral("boardSlot"));
    auto *zoneLayout = new QVBoxLayout(zone);
    zoneLayout->setContentsMargins(0, 0, 0, 0);
    zoneLayout->addWidget(content);
    const int dutyId = duty.dutyId;
    const int memberId = duty.memberId;
    zone->accepts = [dutyId, memberId](const ScheduleDrag::Payload &payload) {
        return !(payload.fromDutyId == dutyId && payload.fromSlot == ScheduleDrag::Serving) && payload.memberId != memberId;
    };
    zone->onDrop = [this, dutyId](const ScheduleDrag::Payload &payload) {
        QTimer::singleShot(0, this, [this, dutyId, payload] {
            emit slotDropRequested(dutyId, ScheduleDrag::Serving, payload.memberId, payload.fromDutyId, payload.fromSlot);
        });
    };
    return zone;
}

// "Backup" and a small chip -- or, when editable and there's none, a
// dashed line to drop one on.
QWidget *ScheduleBoard::buildBackup(const BoardDuty &duty, bool editable, QWidget *parent)
{
    const bool filled = duty.supportMemberId > 0 && !duty.supportMemberName.isEmpty();
    BoardDropZone *zone = editable ? new BoardDropZone(parent) : nullptr;
    QWidget *line = zone ? static_cast<QWidget *>(zone) : new QWidget(parent);
    auto *layout = new QHBoxLayout(line);
    layout->setContentsMargins(4, 0, 0, 0);
    layout->setSpacing(4);
    auto *caption = new QLabel(tr("Backup"), line);
    caption->setObjectName(QStringLiteral("dutyCaption"));
    layout->addWidget(caption);
    if (filled) {
        auto *chip = new BoardMemberChip(duty.supportMemberName, duty.supportMemberColor, line, 18);
        chip->setObjectName(QStringLiteral("boardChipSmall"));
        chip->setTag(duty.supportTag, duty.supportTagToolTip);
        if (editable) {
            const int dutyId = duty.dutyId;
            chip->setDraggable({duty.supportMemberId, dutyId, ScheduleDrag::Backup});
            chip->setRemovable(tr("Remove %1 as backup").arg(duty.supportMemberName), [this, dutyId] {
                QTimer::singleShot(0, this, [this, dutyId] { emit removeRequested(dutyId, ScheduleDrag::Backup); });
            });
        }
        layout->addWidget(chip, 1);
    } else {
        auto *hint = new QLabel(editable ? tr("drop one here") : QStringLiteral("—"), line);
        hint->setObjectName(QStringLiteral("boardHint"));
        layout->addWidget(hint, 1);
    }
    if (!zone) {
        return line;
    }
    zone->setObjectName(filled ? QStringLiteral("boardSlot") : QStringLiteral("boardEmptySlot"));
    const int dutyId = duty.dutyId;
    const int memberId = duty.supportMemberId;
    zone->accepts = [dutyId, memberId](const ScheduleDrag::Payload &payload) {
        return !(payload.fromDutyId == dutyId && payload.fromSlot == ScheduleDrag::Backup) && payload.memberId != memberId;
    };
    zone->onDrop = [this, dutyId](const ScheduleDrag::Payload &payload) {
        QTimer::singleShot(0, this, [this, dutyId, payload] {
            emit slotDropRequested(dutyId, ScheduleDrag::Backup, payload.memberId, payload.fromDutyId, payload.fromSlot);
        });
    };
    return zone;
}

// --- MemberPalette ----------------------------------------------------------

MemberPalette::MemberPalette(QWidget *parent)
    : QWidget(parent)
{
    auto *frame = new BoardDropZone(this);
    frame->setObjectName(QStringLiteral("boardPalette"));
    frame->accepts = [](const ScheduleDrag::Payload &payload) { return payload.fromDutyId > 0; };
    frame->onDrop = [this](const ScheduleDrag::Payload &payload) {
        QTimer::singleShot(0, this, [this, payload] { emit removeRequested(payload.fromDutyId, payload.fromSlot); });
    };

    auto *title = new QLabel(tr("Members"), frame);
    title->setObjectName(QStringLiteral("boardColumnTitle"));
    auto *hint = new QLabel(tr("Drag a member onto any upcoming Sunday's duty. Drag them back here to take them off."), frame);
    hint->setObjectName(QStringLiteral("boardHint"));
    hint->setWordWrap(true);

    m_search = new QLineEdit(frame);
    m_search->setPlaceholderText(tr("Search members..."));
    m_search->setClearButtonEnabled(true);
    connect(m_search, &QLineEdit::textChanged, this, &MemberPalette::rebuild);

    m_noMatchLabel = new QLabel(tr("No members match."), frame);
    m_noMatchLabel->setObjectName(QStringLiteral("mutedLabel"));
    m_noMatchLabel->hide();

    auto *listContent = new QWidget;
    listContent->setObjectName(QStringLiteral("boardPaletteList"));
    m_listLayout = new QVBoxLayout(listContent);
    m_listLayout->setContentsMargins(0, 0, 4, 0);
    m_listLayout->setSpacing(6);
    auto *scroll = new QScrollArea(frame);
    scroll->setWidget(listContent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(10, 10, 10, 10);
    frameLayout->setSpacing(8);
    frameLayout->addWidget(title);
    frameLayout->addWidget(hint);
    frameLayout->addWidget(m_search);
    frameLayout->addWidget(m_noMatchLabel);
    frameLayout->addWidget(scroll, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(frame);
    setFixedWidth(210);
}

void MemberPalette::setMembers(const QVector<Entry> &members)
{
    m_members = members;
    rebuild();
}

void MemberPalette::rebuild()
{
    clearLayout(m_listLayout);
    const QString term = m_search->text().trimmed();
    int shown = 0;
    for (const Entry &member : std::as_const(m_members)) {
        if (!term.isEmpty() && !member.name.contains(term, Qt::CaseInsensitive)) {
            continue;
        }
        auto *chip = new BoardMemberChip(member.name, member.color);
        chip->setDetail(member.detail);
        chip->setDraggable({member.id, -1, ScheduleDrag::NoSlot});
        chip->setToolTip(tr("Drag onto a duty cell, or onto someone to replace them"));
        m_listLayout->addWidget(chip);
        ++shown;
    }
    m_listLayout->addStretch();
    m_noMatchLabel->setVisible(shown == 0 && !m_members.isEmpty());
}
