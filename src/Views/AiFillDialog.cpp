#include "AiFillDialog.h"

#include <algorithm>

#include <QApplication>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QHash>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "Controllers/AvailabilityController.h"
#include "Controllers/DutyController.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/UserController.h"
#include "Models/DutyType.h"
#include "Models/User.h"
#include "Scheduling/AiScheduleFiller.h"

namespace
{
    // Proposal rows keep their index into m_proposals here; Sunday headings
    // and unfilled duties have -1.
    constexpr int kProposalRole = Qt::UserRole;

    QListWidgetItem *addInfoRow(QListWidget *list, const QString &text, bool heading)
    {
        auto *item = new QListWidgetItem(text, list);
        item->setFlags(Qt::ItemIsEnabled);
        item->setData(kProposalRole, -1);
        if (heading) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
        return item;
    }
}

AiFillDialog::AiFillDialog(DutyController *dutyController,
                           DutyTypeController *dutyTypeController,
                           UserController *userController,
                           QWidget *parent)
    : FramelessDialog(parent)
    , m_dutyController(dutyController)
    , m_dutyTypeController(dutyTypeController)
    , m_userController(userController)
    , m_network(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("Fill with AI"));
    auto *heading = new QLabel(tr("Fill with AI"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        tr("Suggests who serves on the empty duties of the coming Sundays, from who usually does each duty "
           "and who marked themselves away. Duties already scheduled stay as they are. Nothing is saved "
           "until you add the suggestions."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    m_sundayCount = new QSpinBox(this);
    m_sundayCount->setRange(1, 12);
    m_sundayCount->setValue(4);
    m_sundayCount->setSuffix(tr(" Sundays"));

    m_dutyTypeList = new QListWidget(this);
    m_dutyTypeList->setFixedHeight(120);
    for (const DutyType &dutyType : m_dutyTypeController->allDutyTypes()) {
        auto *item = new QListWidgetItem(dutyType.iconAndName(), m_dutyTypeList);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        item->setData(Qt::UserRole, dutyType.id());
    }

    m_notesEdit = new QPlainTextEdit(this);
    m_notesEdit->setFixedHeight(60);
    m_notesEdit->setPlaceholderText(tr("Optional, e.g. \"Grace leads worship every Sunday in November\""));

    auto *form = new QFormLayout;
    form->addRow(tr("Fill the next"), m_sundayCount);
    form->addRow(tr("Duties"), m_dutyTypeList);
    form->addRow(tr("Notes for the AI"), m_notesEdit);

    m_suggestButton = new QPushButton(tr("Suggest"), this);
    m_suggestButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_suggestButton, &QPushButton::clicked, this, &AiFillDialog::suggestClicked);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("mutedLabel"));
    m_statusLabel->setWordWrap(true);

    m_proposalList = new QListWidget(this);
    m_proposalList->setMinimumSize(460, 220);
    m_proposalList->hide();
    connect(m_proposalList, &QListWidget::itemChanged, this, &AiFillDialog::updateAddButton);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_addButton = buttons->button(QDialogButtonBox::Ok);
    m_addButton->setText(tr("Add Duties"));
    m_addButton->setEnabled(false);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addWidget(subtitle);
    layout->addLayout(form);
    layout->addWidget(m_suggestButton, 0, Qt::AlignRight);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_proposalList, 1);
    layout->addWidget(buttons);
}

void AiFillDialog::suggestClicked()
{
    QVector<int> dutyTypeIds;
    for (int i = 0; i < m_dutyTypeList->count(); ++i) {
        const QListWidgetItem *item = m_dutyTypeList->item(i);
        if (item->checkState() == Qt::Checked) {
            dutyTypeIds.append(item->data(Qt::UserRole).toInt());
        }
    }
    if (dutyTypeIds.isEmpty()) {
        m_statusLabel->setText(tr("Tick at least one duty to fill."));
        return;
    }

    m_suggestButton->setEnabled(false);
    m_addButton->setEnabled(false);
    m_statusLabel->setText(tr("Asking the AI... this can take up to a minute."));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const AvailabilityController availability;
    const AiSchedule::Result result = AiSchedule::propose(
        *m_network, *m_dutyController, *m_dutyTypeController, *m_userController, availability,
        AiSchedule::upcomingSundays(m_sundayCount->value()), dutyTypeIds, m_notesEdit->toPlainText());
    QApplication::restoreOverrideCursor();
    m_suggestButton->setEnabled(true);
    m_suggestButton->setText(tr("Suggest Again"));

    m_proposals.clear();
    m_proposalList->blockSignals(true);
    m_proposalList->clear();
    m_proposalList->blockSignals(false);
    if (!result.error.isEmpty()) {
        m_proposalList->hide();
        m_statusLabel->setText(result.error);
        updateAddButton();
        return;
    }
    if (result.proposals.isEmpty() && result.unfilled.isEmpty()) {
        m_proposalList->hide();
        m_statusLabel->setText(tr("Those duties are already filled on every one of those Sundays."));
        updateAddButton();
        return;
    }

    QHash<int, QString> names;
    for (const User &user : m_userController->allUsers()) {
        names.insert(user.id(), user.name());
    }
    const auto dutyName = [this](int dutyTypeId) {
        return m_dutyTypeController->dutyTypeById(dutyTypeId).iconAndName();
    };

    // A heading per Sunday, then its proposals, then its unfilled duties.
    QVector<QDate> dates;
    for (const AiSchedule::Proposal &proposal : result.proposals) {
        if (!dates.contains(proposal.date)) {
            dates.append(proposal.date);
        }
    }
    for (const AiSchedule::Slot &slot : result.unfilled) {
        if (!dates.contains(slot.date)) {
            dates.append(slot.date);
        }
    }
    std::sort(dates.begin(), dates.end());

    m_proposalList->blockSignals(true);
    for (const QDate &date : std::as_const(dates)) {
        addInfoRow(m_proposalList, QLocale().toString(date, QStringLiteral("dddd d MMMM yyyy")), true);
        for (const AiSchedule::Proposal &proposal : result.proposals) {
            if (proposal.date != date) {
                continue;
            }
            QString text = tr("%1: %2").arg(dutyName(proposal.dutyTypeId), names.value(proposal.memberId));
            if (proposal.supportMemberId > 0) {
                text += tr("  (Backup: %1)").arg(names.value(proposal.supportMemberId));
            }
            auto *item = new QListWidgetItem(text, m_proposalList);
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            item->setData(kProposalRole, m_proposals.size());
            m_proposals.append(Duty(-1, proposal.dutyTypeId, proposal.date, proposal.memberId,
                                    proposal.supportMemberId, QString()));
        }
        for (const AiSchedule::Slot &slot : result.unfilled) {
            if (slot.date == date) {
                addInfoRow(m_proposalList, tr("%1: nobody suitable is free").arg(dutyName(slot.dutyTypeId)), false)
                    ->setForeground(palette().color(QPalette::PlaceholderText));
            }
        }
    }
    m_proposalList->blockSignals(false);
    m_proposalList->show();

    QString status = tr("%n suggestion(s). Untick any you don't want, then add them.", nullptr, result.proposals.size());
    if (!result.unfilled.isEmpty()) {
        status += QLatin1Char(' ') + tr("%n duty(s) couldn't be filled.", nullptr, result.unfilled.size());
    }
    m_statusLabel->setText(status);
    updateAddButton();
}

void AiFillDialog::updateAddButton()
{
    const int count = duties().size();
    m_addButton->setEnabled(count > 0);
    m_addButton->setText(count > 0 ? tr("Add %n Duty(s)", nullptr, count) : tr("Add Duties"));
}

QVector<Duty> AiFillDialog::duties() const
{
    QVector<Duty> chosen;
    for (int i = 0; i < m_proposalList->count(); ++i) {
        const QListWidgetItem *item = m_proposalList->item(i);
        const int index = item->data(kProposalRole).toInt();
        if (index >= 0 && item->checkState() == Qt::Checked) {
            chosen.append(m_proposals[index]);
        }
    }
    return chosen;
}
