#pragma once

#include <QVector>
#include <QWidget>

#include "Models/AccessRights.h"
#include "Models/Feedback.h"

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class FeedbackController;
class SuggestLineEdit;
class UserController;

// "Feedback" tab: send a bug report, a feature request, or a change
// wanted on a member profile -- what kind, who it's from (optional), a
// subject and the details. Saved in the feedback table.
//
// Follows the Feedback rights in Settings > Access Rights: Create sends,
// Update shows everyone's requests (open first) to read and Mark Done or
// Reopen, Delete removes them.
class FeedbackView : public QWidget
{
    Q_OBJECT

public:
    FeedbackView(FeedbackController *feedbackController, UserController *userController, QWidget *parent = nullptr);

public slots:
    void refresh();
    void setAccess(const SectionAccess &access);

private slots:
    void sendClicked();
    void selectionChanged();
    void toggleDoneClicked();
    void deleteClicked();

private:
    void rebuildList();
    void updateActionState();
    // nullptr when nothing is selected.
    const Feedback *selectedFeedback() const;
    QString memberName(int memberId) const;
    // Shows text under the form, red for a problem, gold for a confirmation.
    void showFormMessage(const QString &text, bool isError);
    static QString kindLabel(Feedback::Kind kind);

    FeedbackController *m_feedbackController = nullptr;
    UserController *m_userController = nullptr;
    QVector<Feedback> m_feedback;
    QList<QPair<int, QString>> m_memberNames;
    SectionAccess m_access;

    QGroupBox *m_formBox = nullptr;
    QComboBox *m_kindCombo = nullptr;
    SuggestLineEdit *m_fromEdit = nullptr;
    QLineEdit *m_subjectEdit = nullptr;
    QPlainTextEdit *m_detailsEdit = nullptr;
    QLabel *m_formMessage = nullptr;
    QPushButton *m_sendButton = nullptr;

    QGroupBox *m_requestsBox = nullptr;
    // Shown instead of an empty page when someone can open the tab but
    // neither send nor read requests.
    QLabel *m_noAccessLabel = nullptr;
    QListWidget *m_list = nullptr;
    QLabel *m_requestTitle = nullptr;
    QLabel *m_requestMeta = nullptr;
    QPlainTextEdit *m_requestDetails = nullptr;
    QPushButton *m_doneButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
};
