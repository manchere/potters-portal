#include "SuggestLineEdit.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QStringListModel>

namespace
{
    // An item's text without a leading icon, e.g. "🚪 Ushering" -> "Ushering",
    // so typing just the name still counts as a match.
    QString withoutIcon(const QString &text)
    {
        int start = 0;
        while (start < text.size() && !text.at(start).isLetterOrNumber()) {
            ++start;
        }
        return text.mid(start);
    }
}

SuggestLineEdit::SuggestLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    m_model = new QStringListModel(this);
    m_completer = new QCompleter(m_model, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setMaxVisibleItems(10);
    m_completer->popup()->setObjectName(QStringLiteral("completerPopup"));
    setCompleter(m_completer);
    setClearButtonEnabled(true);
}

void SuggestLineEdit::setItems(const QList<QPair<int, QString>> &items)
{
    m_items = items;
    QStringList texts;
    for (const auto &item : items) {
        texts.append(item.second);
    }
    m_model->setStringList(texts);
}

int SuggestLineEdit::currentId() const
{
    const QString typed = text().trimmed();
    if (typed.isEmpty()) {
        return -1;
    }
    for (const auto &item : m_items) {
        if (item.second.compare(typed, Qt::CaseInsensitive) == 0
            || withoutIcon(item.second).compare(typed, Qt::CaseInsensitive) == 0) {
            return item.first;
        }
    }
    return -1;
}

void SuggestLineEdit::setCurrentId(int id)
{
    for (const auto &item : std::as_const(m_items)) {
        if (item.first == id) {
            setText(item.second);
            return;
        }
    }
    clear();
}

bool SuggestLineEdit::hasUnknownText() const
{
    return !text().trimmed().isEmpty() && currentId() < 0;
}

void SuggestLineEdit::mousePressEvent(QMouseEvent *event)
{
    QLineEdit::mousePressEvent(event);
    showAllIfEmpty();
}

void SuggestLineEdit::showAllIfEmpty()
{
    if (hasFocus() && text().isEmpty() && !m_completer->popup()->isVisible()) {
        m_completer->setCompletionPrefix(QString());
        m_completer->complete();
    }
}
