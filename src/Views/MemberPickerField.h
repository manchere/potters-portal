#pragma once

#include <QFrame>
#include <QList>
#include <QPair>
#include <QVector>

class QCompleter;
class QLineEdit;
class QStringListModel;
class FlowLayout;

// Text field that holds several Members as removable chips. Typing shows
// matching names (anywhere in the name, any case); picking one -- with the
// mouse, or Enter for the best match -- turns it into a chip. Backspace in
// the empty field removes the last chip. Used by the Reports tab's Member
// filter, where no chips means "All members", and -- holding duty types
// instead of Members -- by the Schedule tab's Add Member dialog.
class MemberPickerField : public QFrame
{
    Q_OBJECT

public:
    explicit MemberPickerField(QWidget *parent = nullptr);

    // Every Member that can be picked, as (id, name). Chips for Members no
    // longer in the list are dropped.
    void setMembers(const QList<QPair<int, QString>> &members);

    // Hint shown with no chips, and with some.
    void setPlaceholders(const QString &empty, const QString &more);

    QVector<int> selectedIds() const;
    QStringList selectedNames() const;

signals:
    void selectionChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void addMember(int id);
    void removeMember(int id);
    // Picks the best match for the typed text: an exact name, else the
    // first name containing it. Returns false when nothing matches.
    bool addFromTypedText();
    void rebuildChips();
    void updateSuggestions();
    QString nameOf(int id) const;

    QList<QPair<int, QString>> m_members;
    QVector<int> m_selected;
    QString m_emptyPlaceholder = QStringLiteral("All members — type a name to add");
    QString m_morePlaceholder = QStringLiteral("Add another member...");
    // Set between a suggestion being picked and the edit being cleared.
    bool m_justPicked = false;

    FlowLayout *m_flow = nullptr;
    QLineEdit *m_edit = nullptr;
    QCompleter *m_completer = nullptr;
    QStringListModel *m_suggestions = nullptr;
};
