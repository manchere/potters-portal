#include "MemberPickerField.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QStringListModel>
#include <QStyle>
#include <QTimer>
#include <QToolButton>

#include "FlowLayout.h"

MemberPickerField::MemberPickerField(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("memberPickerField"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setCursor(Qt::IBeamCursor);

    m_flow = new FlowLayout(this, 4, 6, 4);

    m_edit = new QLineEdit(this);
    m_edit->setObjectName(QStringLiteral("memberPickerEdit"));
    m_edit->installEventFilter(this);
    m_flow->addWidget(m_edit);

    m_suggestions = new QStringListModel(this);
    m_completer = new QCompleter(m_suggestions, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setMaxVisibleItems(10);
    m_completer->popup()->setObjectName(QStringLiteral("completerPopup"));
    m_edit->setCompleter(m_completer);

    connect(m_completer, qOverload<const QString &>(&QCompleter::activated), this, [this](const QString &name) {
        for (const auto &member : std::as_const(m_members)) {
            if (member.second == name) {
                addMember(member.first);
                break;
            }
        }
        // QCompleter writes the picked name into the edit after this
        // signal, and passes a picking Enter on to the edit too; clear the
        // edit and the guard once both have happened.
        m_justPicked = true;
        QTimer::singleShot(0, this, [this]() {
            m_edit->clear();
            m_justPicked = false;
        });
    });
    connect(m_edit, &QLineEdit::returnPressed, this, [this]() {
        if (m_justPicked) {
            return;
        }
        if (addFromTypedText()) {
            m_edit->clear();
        }
    });

    rebuildChips();
}

void MemberPickerField::setPlaceholders(const QString &empty, const QString &more)
{
    m_emptyPlaceholder = empty;
    m_morePlaceholder = more;
    rebuildChips();
}

void MemberPickerField::setMembers(const QList<QPair<int, QString>> &members)
{
    m_members = members;
    const QVector<int> previous = m_selected;
    m_selected.clear();
    for (int id : previous) {
        if (!nameOf(id).isNull()) {
            m_selected.append(id);
        }
    }
    rebuildChips();
    if (m_selected != previous) {
        emit selectionChanged();
    }
}

QVector<int> MemberPickerField::selectedIds() const
{
    return m_selected;
}

void MemberPickerField::setSelectedIds(const QVector<int> &ids)
{
    m_selected.clear();
    for (int id : ids) {
        if (!nameOf(id).isNull() && !m_selected.contains(id)) {
            m_selected.append(id);
        }
    }
    rebuildChips();
}

QStringList MemberPickerField::selectedNames() const
{
    QStringList names;
    for (int id : m_selected) {
        names.append(nameOf(id));
    }
    return names;
}

bool MemberPickerField::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit) {
        if (event->type() == QEvent::KeyPress) {
            auto *key = static_cast<QKeyEvent *>(event);
            if (key->key() == Qt::Key_Backspace && m_edit->text().isEmpty() && !m_selected.isEmpty()) {
                removeMember(m_selected.last());
                return true;
            }
        } else if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
            // Stylesheets have no :focus-within, so the frame's focus ring
            // follows the edit through a property.
            setProperty("focused", event->type() == QEvent::FocusIn);
            style()->unpolish(this);
            style()->polish(this);
        }
    }
    return QFrame::eventFilter(watched, event);
}

void MemberPickerField::mousePressEvent(QMouseEvent *event)
{
    m_edit->setFocus();
    QFrame::mousePressEvent(event);
}

void MemberPickerField::addMember(int id)
{
    if (id <= 0 || m_selected.contains(id)) {
        return;
    }
    m_selected.append(id);
    rebuildChips();
    emit selectionChanged();
}

void MemberPickerField::removeMember(int id)
{
    if (m_selected.removeAll(id) == 0) {
        return;
    }
    rebuildChips();
    emit selectionChanged();
}

bool MemberPickerField::addFromTypedText()
{
    const QString typed = m_edit->text().trimmed();
    if (typed.isEmpty()) {
        return false;
    }
    int match = -1;
    for (const auto &member : std::as_const(m_members)) {
        if (m_selected.contains(member.first)) {
            continue;
        }
        if (member.second.compare(typed, Qt::CaseInsensitive) == 0) {
            match = member.first;
            break;
        }
        if (match < 0 && member.second.contains(typed, Qt::CaseInsensitive)) {
            match = member.first;
        }
    }
    if (match < 0) {
        return false;
    }
    addMember(match);
    return true;
}

void MemberPickerField::rebuildChips()
{
    // Take everything out but keep the edit; chips are rebuilt in order.
    while (QLayoutItem *item = m_flow->takeAt(0)) {
        if (item->widget() && item->widget() != m_edit) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    for (int id : std::as_const(m_selected)) {
        auto *chip = new QFrame(this);
        chip->setObjectName(QStringLiteral("memberChip"));
        chip->setCursor(Qt::ArrowCursor);
        auto *chipLayout = new QHBoxLayout(chip);
        chipLayout->setContentsMargins(10, 2, 4, 2);
        chipLayout->setSpacing(2);
        auto *label = new QLabel(nameOf(id), chip);
        label->setObjectName(QStringLiteral("memberChipLabel"));
        auto *remove = new QToolButton(chip);
        remove->setObjectName(QStringLiteral("memberChipRemove"));
        remove->setText(QStringLiteral("×"));
        remove->setToolTip(tr("Remove %1").arg(nameOf(id)));
        remove->setCursor(Qt::PointingHandCursor);
        connect(remove, &QToolButton::clicked, this, [this, id]() { removeMember(id); });
        chipLayout->addWidget(label);
        chipLayout->addWidget(remove);
        m_flow->addWidget(chip);
    }
    m_flow->addWidget(m_edit);

    m_edit->setPlaceholderText(m_selected.isEmpty() ? m_emptyPlaceholder : m_morePlaceholder);
    // The flow layout doesn't stretch the edit, so give it room for the
    // whole placeholder.
    m_edit->setMinimumWidth(qMax(160, m_edit->fontMetrics().horizontalAdvance(m_edit->placeholderText()) + 48));
    updateSuggestions();
    updateGeometry();
}

void MemberPickerField::updateSuggestions()
{
    // Members already picked aren't offered again.
    QStringList names;
    for (const auto &member : std::as_const(m_members)) {
        if (!m_selected.contains(member.first)) {
            names.append(member.second);
        }
    }
    m_suggestions->setStringList(names);
}

QString MemberPickerField::nameOf(int id) const
{
    for (const auto &member : m_members) {
        if (member.first == id) {
            return member.second;
        }
    }
    return QString();
}
