#include "FeedbackView.h"

#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "ActionBar.h"
#include "Controllers/FeedbackController.h"
#include "Controllers/UserController.h"
#include "SuggestLineEdit.h"

FeedbackView::FeedbackView(FeedbackController *feedbackController, UserController *userController, QWidget *parent)
    : QWidget(parent)
    , m_feedbackController(feedbackController)
    , m_userController(userController)
{
    auto *title = new QLabel(tr("Feedback"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        tr("Report a bug, suggest a feature, or ask for a change to a member profile. "
           "An Admin will read it and follow up."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    // --- New request form ---------------------------------------------------
    m_kindCombo = new QComboBox(this);
    for (Feedback::Kind kind : {Feedback::Kind::Bug, Feedback::Kind::Feature, Feedback::Kind::Profile}) {
        m_kindCombo->addItem(kindLabel(kind), static_cast<int>(kind));
    }
    m_fromEdit = new SuggestLineEdit(this);
    m_fromEdit->setPlaceholderText(tr("Your name (optional)"));
    m_subjectEdit = new QLineEdit(this);
    m_subjectEdit->setPlaceholderText(tr("A short summary"));
    m_detailsEdit = new QPlainTextEdit(this);
    m_detailsEdit->setPlaceholderText(tr("What happened, what you'd like, or what should change on the profile"));
    m_formMessage = new QLabel(this);
    m_formMessage->setWordWrap(true);
    connect(m_subjectEdit, &QLineEdit::textChanged, m_formMessage, &QLabel::clear);
    connect(m_fromEdit, &QLineEdit::textChanged, m_formMessage, &QLabel::clear);
    connect(m_kindCombo, &QComboBox::currentIndexChanged, this, [this]() {
        // A profile change is about someone, so ask who.
        const bool profile = m_kindCombo->currentData().toInt() == static_cast<int>(Feedback::Kind::Profile);
        m_fromEdit->setPlaceholderText(profile ? tr("Whose profile? (your name)") : tr("Your name (optional)"));
    });

    m_sendButton = new QPushButton(tr("Send"), this);
    connect(m_sendButton, &QPushButton::clicked, this, &FeedbackView::sendClicked);

    auto *form = new QFormLayout;
    form->addRow(tr("Type"), m_kindCombo);
    form->addRow(tr("From"), m_fromEdit);
    form->addRow(tr("Subject"), m_subjectEdit);
    form->addRow(tr("Details"), m_detailsEdit);
    auto *sendRow = new QHBoxLayout;
    sendRow->addWidget(m_formMessage, 1);
    sendRow->addWidget(m_sendButton);
    m_formBox = new QGroupBox(tr("Send a request"), this);
    auto *formLayout = new QVBoxLayout(m_formBox);
    formLayout->addLayout(form, 1);
    formLayout->addLayout(sendRow);

    // --- Sent requests (Admin) ---------------------------------------------
    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    connect(m_list, &QListWidget::currentRowChanged, this, &FeedbackView::selectionChanged);
    m_requestTitle = new QLabel(this);
    m_requestTitle->setStyleSheet(QStringLiteral("font-weight: 700;"));
    m_requestTitle->setWordWrap(true);
    m_requestMeta = new QLabel(this);
    m_requestMeta->setObjectName(QStringLiteral("mutedLabel"));
    m_requestMeta->setWordWrap(true);
    m_requestDetails = new QPlainTextEdit(this);
    m_requestDetails->setReadOnly(true);
    m_requestDetails->setMaximumHeight(140);

    m_requestsBox = new QGroupBox(tr("Requests"), this);
    auto *requestsLayout = new QVBoxLayout(m_requestsBox);
    requestsLayout->addWidget(m_list, 1);
    requestsLayout->addWidget(m_requestTitle);
    requestsLayout->addWidget(m_requestMeta);
    requestsLayout->addWidget(m_requestDetails);

    m_doneButton = new QPushButton(tr("Mark Done"), this);
    m_doneButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_doneButton, &QPushButton::clicked, this, &FeedbackView::toggleDoneClicked);
    m_deleteButton = new QPushButton(tr("Delete"), this);
    m_deleteButton->setObjectName(QStringLiteral("dangerButton"));
    connect(m_deleteButton, &QPushButton::clicked, this, &FeedbackView::deleteClicked);
    auto *actionBar = new ActionBar(this);
    actionBar->addWidget(m_doneButton);
    actionBar->addStretch();
    actionBar->addWidget(m_deleteButton);

    m_noAccessLabel = new QLabel(
        tr("You can't send or read requests here yet. Ask an Admin if you need access."), this);
    m_noAccessLabel->setObjectName(QStringLiteral("mutedLabel"));
    m_noAccessLabel->setAlignment(Qt::AlignCenter);
    m_noAccessLabel->setWordWrap(true);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(m_noAccessLabel, 1);
    columns->addWidget(m_formBox, 1);
    columns->addWidget(m_requestsBox, 1);

    auto *content = new QVBoxLayout;
    content->setSpacing(12);
    content->addWidget(title);
    content->addWidget(subtitle);
    content->addSpacing(6);
    content->addLayout(columns, 1);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    layout->addLayout(content, 1);
    layout->addWidget(actionBar);

    setAccess(SectionAccess());
    refresh();
}

void FeedbackView::showFormMessage(const QString &text, bool isError)
{
    m_formMessage->setObjectName(isError ? QStringLiteral("fieldError") : QStringLiteral("accentLabel"));
    m_formMessage->style()->unpolish(m_formMessage);
    m_formMessage->style()->polish(m_formMessage);
    m_formMessage->setText(text);
}

QString FeedbackView::kindLabel(Feedback::Kind kind)
{
    switch (kind) {
    case Feedback::Kind::Feature: return QStringLiteral("\U0001F4A1  ") + tr("Feature request");
    case Feedback::Kind::Profile: return QStringLiteral("\U0001F464  ") + tr("Profile change");
    case Feedback::Kind::Bug: break;
    }
    return QStringLiteral("\U0001F41E  ") + tr("Bug report");
}

void FeedbackView::setAccess(const SectionAccess &access)
{
    m_access = access;
    m_formBox->setVisible(access.create);
    m_requestsBox->setVisible(access.update || access.remove);
    m_noAccessLabel->setVisible(!access.create && !access.update && !access.remove);
    m_doneButton->setVisible(access.update);
    m_deleteButton->setVisible(access.remove);
    refresh();
    updateActionState();
}

void FeedbackView::refresh()
{
    m_memberNames.clear();
    for (const User &user : m_userController->allUsers()) {
        m_memberNames.append({user.id(), user.name()});
    }
    const int fromId = m_fromEdit->currentId();
    m_fromEdit->setItems(m_memberNames);
    if (fromId > 0) {
        m_fromEdit->setCurrentId(fromId);
    }
    // Only fetched for someone allowed to see them.
    m_feedback = m_access.update || m_access.remove ? m_feedbackController->allFeedback() : QVector<Feedback>();
    rebuildList();
}

QString FeedbackView::memberName(int memberId) const
{
    for (const auto &member : m_memberNames) {
        if (member.first == memberId) {
            return member.second;
        }
    }
    return QString();
}

void FeedbackView::rebuildList()
{
    const Feedback *previous = selectedFeedback();
    const int previousId = previous ? previous->id() : -1;
    m_list->clear();
    for (const Feedback &feedback : std::as_const(m_feedback)) {
        QString text = kindLabel(feedback.kind()) + QStringLiteral("  ·  ") + feedback.subject();
        if (feedback.isDone()) {
            text = tr("✓ Done  ·  %1").arg(text);
        }
        auto *item = new QListWidgetItem(text, m_list);
        item->setData(Qt::UserRole, feedback.id());
        if (feedback.isDone()) {
            QFont font = item->font();
            font.setStrikeOut(true);
            item->setFont(font);
            item->setForeground(m_list->palette().color(QPalette::Disabled, QPalette::Text));
        }
        if (feedback.id() == previousId) {
            m_list->setCurrentItem(item);
        }
    }
    if (m_feedback.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No requests yet."), m_list);
        item->setFlags(Qt::NoItemFlags);
    }
    selectionChanged();
}

const Feedback *FeedbackView::selectedFeedback() const
{
    const QListWidgetItem *item = m_list ? m_list->currentItem() : nullptr;
    if (!item || !item->data(Qt::UserRole).isValid()) {
        return nullptr;
    }
    const int id = item->data(Qt::UserRole).toInt();
    for (const Feedback &feedback : m_feedback) {
        if (feedback.id() == id) {
            return &feedback;
        }
    }
    return nullptr;
}

void FeedbackView::selectionChanged()
{
    const Feedback *feedback = selectedFeedback();
    m_requestTitle->setVisible(feedback);
    m_requestMeta->setVisible(feedback);
    m_requestDetails->setVisible(feedback);
    if (feedback) {
        m_requestTitle->setText(feedback->subject());
        const QString from = memberName(feedback->memberId());
        const QString when = QLocale().toString(feedback->createdAt().toLocalTime(), QLocale::ShortFormat);
        m_requestMeta->setText(from.isEmpty()
            ? tr("%1  ·  sent %2").arg(kindLabel(feedback->kind()), when)
            : tr("%1  ·  from %2  ·  sent %3").arg(kindLabel(feedback->kind()), from, when));
        m_requestDetails->setPlainText(feedback->details());
    }
    updateActionState();
}

void FeedbackView::updateActionState()
{
    const Feedback *feedback = selectedFeedback();
    m_doneButton->setEnabled(m_access.update && feedback);
    m_deleteButton->setEnabled(m_access.remove && feedback);
    m_doneButton->setText(feedback && feedback->isDone() ? tr("Reopen") : tr("Mark Done"));
}

void FeedbackView::sendClicked()
{
    if (!m_access.create) {
        return;
    }
    const QString subject = m_subjectEdit->text().trimmed();
    if (subject.isEmpty()) {
        showFormMessage(tr("Add a short subject."), true);
        m_subjectEdit->setFocus();
        return;
    }
    if (m_fromEdit->hasUnknownText()) {
        showFormMessage(tr("No member is called \"%1\" -- pick a name from the suggestions, or leave it empty.")
                            .arg(m_fromEdit->text().trimmed()), true);
        m_fromEdit->setFocus();
        return;
    }

    Feedback feedback;
    feedback.setKind(static_cast<Feedback::Kind>(m_kindCombo->currentData().toInt()));
    feedback.setMemberId(m_fromEdit->currentId());
    feedback.setSubject(subject);
    feedback.setDetails(m_detailsEdit->toPlainText().trimmed());
    if (!m_feedbackController->addFeedback(feedback)) {
        QMessageBox::critical(this, tr("Send Request"), m_feedbackController->lastError());
        return;
    }
    m_subjectEdit->clear();
    m_detailsEdit->clear();
    showFormMessage(tr("Thanks! Your request was sent."), false);
    refresh();
}

void FeedbackView::toggleDoneClicked()
{
    const Feedback *feedback = selectedFeedback();
    if (!m_access.update || !feedback) {
        return;
    }
    if (!m_feedbackController->setDone(feedback->id(), !feedback->isDone())) {
        QMessageBox::critical(this, tr("Feedback"), m_feedbackController->lastError());
        return;
    }
    refresh();
}

void FeedbackView::deleteClicked()
{
    const Feedback *feedback = selectedFeedback();
    if (!m_access.remove || !feedback) {
        return;
    }
    if (QMessageBox::question(this, tr("Delete Request"), tr("Delete \"%1\"?").arg(feedback->subject()))
        != QMessageBox::Yes) {
        return;
    }
    if (!m_feedbackController->removeFeedback(feedback->id())) {
        QMessageBox::critical(this, tr("Delete Request"), m_feedbackController->lastError());
        return;
    }
    refresh();
}
