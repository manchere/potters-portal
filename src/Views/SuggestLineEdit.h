#pragma once

#include <QLineEdit>
#include <QList>
#include <QPair>

class QCompleter;
class QStringListModel;

// A text field that picks one item from a fixed list: typing suggests the
// items containing the text (any case), and clicking into the empty field
// lists them all. Used in place of a dropdown on the Assign Duty dialog.
class SuggestLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit SuggestLineEdit(QWidget *parent = nullptr);

    // (id, text) pairs to choose from, in display order.
    void setItems(const QList<QPair<int, QString>> &items);

    // The id whose text matches what's typed (any case, ignoring outer
    // spaces and a leading icon); -1 when the field is empty or matches
    // nothing.
    int currentId() const;
    void setCurrentId(int id);

    // True when something is typed but it isn't one of the items.
    bool hasUnknownText() const;

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void showAllIfEmpty();

    QList<QPair<int, QString>> m_items;
    QCompleter *m_completer = nullptr;
    QStringListModel *m_model = nullptr;
};
