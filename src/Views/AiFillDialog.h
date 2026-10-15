#pragma once

#include "FramelessDialog.h"

#include <QVector>

#include "Models/Duty.h"

class QLabel;
class QListWidget;
class QNetworkAccessManager;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class DutyController;
class DutyTypeController;
class UserController;

// "Fill with AI" on the Schedule tab: the Admin picks how many upcoming
// Sundays and which duties, adds any notes for the AI, and gets
// a proposal for every empty duty (Scheduling/AiScheduleFiller). Each
// proposal can be unticked before "Add Duties"; duties() is what's left
// ticked, for ScheduleTab to save. Duties already on those Sundays are
// never touched.
class AiFillDialog : public FramelessDialog
{
    Q_OBJECT

public:
    AiFillDialog(DutyController *dutyController,
                 DutyTypeController *dutyTypeController,
                 UserController *userController,
                 QWidget *parent = nullptr);

    // The ticked proposals, as new duties (id() < 0).
    QVector<Duty> duties() const;

private slots:
    void suggestClicked();
    void updateAddButton();

private:
    DutyController *m_dutyController = nullptr;
    DutyTypeController *m_dutyTypeController = nullptr;
    UserController *m_userController = nullptr;
    QNetworkAccessManager *m_network = nullptr;

    QSpinBox *m_sundayCount = nullptr;
    QListWidget *m_dutyTypeList = nullptr;
    QPlainTextEdit *m_notesEdit = nullptr;
    QPushButton *m_suggestButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QListWidget *m_proposalList = nullptr;
    QPushButton *m_addButton = nullptr;
    // One per proposal row, in the same order (unfilled rows have none).
    QVector<Duty> m_proposals;
};
