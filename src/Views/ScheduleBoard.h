#pragma once

#include <QDate>
#include <QFrame>
#include <QString>
#include <QVector>
#include <QWidget>

#include <functional>

#include "Models/DutyType.h"

class QGridLayout;
class QLabel;
class QLineEdit;
class QMimeData;
class QVBoxLayout;

// One duty as a schedule board shows it: the Duty with its names and
// colors already looked up, so the board never queries anything. Built by
// the Schedule tab (from Duty) and the Reports tab (from ScheduleReportRow).
struct BoardDuty
{
    int dutyId = -1;
    QDate serviceDate;
    // -1 when unknown (report rows); the column is then matched by name.
    int dutyTypeId = -1;
    QString dutyTypeName;
    QString dutyTypeIcon;
    // -1 / empty when nobody is serving or there's no backup.
    int memberId = -1;
    QString memberName;
    QString memberColor;
    int supportMemberId = -1;
    QString supportMemberName;
    QString supportMemberColor;
    QString notes;
    // A short tag beside a name, e.g. "Marked away" (empty for none), and
    // what its tooltip says.
    QString memberTag;
    QString memberTagToolTip;
    QString supportTag;
    QString supportTagToolTip;
};

// What's being dragged on a board: a member, either from the member
// palette (fromDutyId -1) or out of one of a duty's two slots.
namespace ScheduleDrag
{
    enum Slot
    {
        NoSlot = 0,
        Serving = 1,
        Backup = 2,
    };

    struct Payload
    {
        int memberId = -1;
        int fromDutyId = -1;
        int fromSlot = NoSlot;
    };

    QMimeData *encode(const Payload &payload);
    bool decode(const QMimeData *mime, Payload *payload);
}

// A frame that takes board drags. It highlights while one hovers over it
// (the "dropHover" property, styled in Style.cpp) and hands the payload to
// onDrop. accepts, when set, can turn particular payloads away.
class BoardDropZone : public QFrame
{
    Q_OBJECT

public:
    explicit BoardDropZone(QWidget *parent = nullptr);

    std::function<bool(const ScheduleDrag::Payload &)> accepts;
    std::function<void(const ScheduleDrag::Payload &)> onDrop;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setHover(bool hover);
};

// A member's badge and name as a small card. When given a payload it can
// be dragged; with onRemove it also has a "×" button.
class BoardMemberChip : public QFrame
{
    Q_OBJECT

public:
    BoardMemberChip(const QString &name, const QString &color, QWidget *parent = nullptr, int badgeSize = 26);

    // A small tag after the name (see BoardDuty::memberTag).
    void setTag(const QString &tag, const QString &toolTip);
    // Muted text under the name, e.g. "2 this month".
    void setDetail(const QString &detail);
    void setDraggable(const ScheduleDrag::Payload &payload);
    void setRemovable(const QString &toolTip, std::function<void()> onRemove);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QVBoxLayout *m_textLayout = nullptr;
    QWidget *m_nameRow = nullptr;
    bool m_draggable = false;
    ScheduleDrag::Payload m_payload;
    QPoint m_pressPos;
};

// Several Sundays' schedules as one grid, like a serving rota sheet: a
// row per Sunday (under a heading for each month), a column per duty type.
// Each cell holds who serves that duty that Sunday, with their backup
// beneath. Used read-only (Reports) and editable (the Schedule tab's
// board), where whole months can be filled in one place.
//
// In an editable row, members can be dragged:
//  - onto a cell: they take on that duty that Sunday (addRequested);
//  - onto someone already in a cell, or onto a backup line: they take that
//    slot; dragged out of another slot, whoever was there swaps into it
//    (slotDropRequested) -- across Sundays too;
//  - back onto the member palette, or "×": off the slot (removeRequested).
// The board only reports what was dropped where; the owner decides what
// that means and rebuilds the board. The signals arrive after the drop
// has finished, so the board can safely be rebuilt in response.
class ScheduleBoard : public QWidget
{
    Q_OBJECT

public:
    explicit ScheduleBoard(QWidget *parent = nullptr);

    // dates: the rows, in the order given. columns: the duty types, in
    // order; when empty, the duty types the duties use (by name).
    void setSchedule(const QVector<QDate> &dates, const QVector<DutyType> &columns, const QVector<BoardDuty> &duties);
    // Rows from editableFrom on take drags; earlier ones stay read-only.
    // This and setHighlightedDate apply from the next setSchedule.
    void setEditable(bool editable, const QDate &editableFrom = QDate());
    // Marks one row (e.g. the Sunday picked in the list).
    void setHighlightedDate(const QDate &date);

signals:
    void addRequested(const QDate &date, int dutyTypeId, int memberId, int fromDutyId, int fromSlot);
    void slotDropRequested(int dutyId, int slot, int memberId, int fromDutyId, int fromSlot);
    void removeRequested(int dutyId, int slot);
    // A row's date was clicked.
    void dateActivated(const QDate &date);

private:
    void rebuild();
    bool rowEditable(const QDate &date) const;
    QWidget *buildCell(const QDate &date, int dutyTypeId, const QVector<BoardDuty> &duties, QWidget *parent);
    QWidget *buildDutyBlock(const BoardDuty &duty, bool editable, QWidget *parent);
    QWidget *buildServing(const BoardDuty &duty, bool editable, QWidget *parent);
    QWidget *buildBackup(const BoardDuty &duty, bool editable, QWidget *parent);

    QVBoxLayout *m_layout = nullptr;
    QWidget *m_grid = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QVector<QDate> m_dates;
    QVector<DutyType> m_columns;
    QVector<BoardDuty> m_duties;
    bool m_editable = false;
    QDate m_editableFrom;
    QDate m_highlightedDate;
};

// The members an Admin drags onto a ScheduleBoard, with a search box.
// Each can say how often they're already on (detail) and carry a tag.
// Dropping a slot's member back here takes them off that slot
// (removeRequested).
class MemberPalette : public QWidget
{
    Q_OBJECT

public:
    struct Entry
    {
        int id = -1;
        QString name;
        QString color;
        QString detail;
    };

    explicit MemberPalette(QWidget *parent = nullptr);

    void setMembers(const QVector<Entry> &members);

signals:
    void removeRequested(int dutyId, int slot);

private:
    void rebuild();

    QLineEdit *m_search = nullptr;
    QVBoxLayout *m_listLayout = nullptr;
    QLabel *m_noMatchLabel = nullptr;
    QVector<Entry> m_members;
};
