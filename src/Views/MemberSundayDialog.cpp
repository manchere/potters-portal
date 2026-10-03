#include "MemberSundayDialog.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

#include "Controllers/DutyTypeController.h"
#include "Controllers/UserController.h"
#include "MemberBadge.h"
#include "Models/DutyType.h"

MemberSundayDialog::MemberSundayDialog(
    const User &member,
    const QString &teamName,
    const QDate &sunday,
    const QVector<Duty> &sundayDuties,
    bool markedAway,
    bool canEdit,
    UserController *userController,
    DutyTypeController *dutyTypeController,
    QWidget *parent)
    : FramelessDialog(parent)
{
    setWindowTitle(member.name());

    // --- Who ------------------------------------------------------------------
    auto *nameLabel = new QLabel(member.name(), this);
    nameLabel->setObjectName(QStringLiteral("pageTitle"));
    auto *detailsLabel = new QLabel(this);
    detailsLabel->setObjectName(QStringLiteral("mutedLabel"));
    QStringList details;
    if (!teamName.isEmpty()) {
        details << tr("Team: %1").arg(teamName);
    }
    if (!member.phone().isEmpty()) {
        details << member.phone();
    }
    detailsLabel->setText(details.join(QStringLiteral("  ·  ")));
    detailsLabel->setVisible(!details.isEmpty());
    auto *dateLabel = new QLabel(QLocale().toString(sunday, QStringLiteral("dddd d MMMM yyyy")), this);
    dateLabel->setObjectName(QStringLiteral("pageSubtitle"));

    auto *whoText = new QVBoxLayout;
    whoText->setSpacing(2);
    whoText->addWidget(nameLabel);
    whoText->addWidget(detailsLabel);
    whoText->addWidget(dateLabel);
    auto *whoRow = new QHBoxLayout;
    whoRow->setSpacing(14);
    whoRow->addWidget(MemberBadge::make(member.name(), member.color(), 56, this), 0, Qt::AlignTop);
    whoRow->addLayout(whoText, 1);

    auto *layout = contentLayout();
    layout->addLayout(whoRow);

    if (markedAway) {
        auto *away = new QLabel(tr("Marked themselves away this Sunday."), this);
        away->setObjectName(QStringLiteral("accentLabel"));
        away->setWordWrap(true);
        layout->addWidget(away);
    }

    // --- Their part this Sunday --------------------------------------------------
    auto memberName = [userController](int id) {
        return id > 0 ? userController->userById(id).name() : QString();
    };
    auto addLine = [this](QVBoxLayout *box, const QString &title, const QString &detail) {
        auto *titleLabel = new QLabel(title, this);
        titleLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));
        titleLabel->setWordWrap(true);
        box->addWidget(titleLabel);
        if (!detail.isEmpty()) {
            auto *detailLabel = new QLabel(detail, this);
            detailLabel->setObjectName(QStringLiteral("mutedLabel"));
            detailLabel->setWordWrap(true);
            box->addWidget(detailLabel);
        }
    };

    auto *servingBox = new QGroupBox(tr("Serving"), this);
    auto *servingLayout = new QVBoxLayout(servingBox);
    auto *backingBox = new QGroupBox(tr("Backing up"), this);
    auto *backingLayout = new QVBoxLayout(backingBox);
    int serving = 0;
    int backing = 0;
    for (const Duty &duty : sundayDuties) {
        const QString dutyName = dutyTypeController->dutyTypeById(duty.dutyTypeId()).iconAndName();
        if (duty.memberId() == member.id()) {
            QStringList detail;
            const QString backup = memberName(duty.supportMemberId());
            detail << (backup.isEmpty() ? tr("No backup") : tr("Backup: %1").arg(backup));
            if (!duty.notes().isEmpty()) {
                detail << duty.notes();
            }
            addLine(servingLayout, dutyName, detail.join(QStringLiteral("  ·  ")));
            ++serving;
        } else if (duty.supportMemberId() == member.id()) {
            const QString main = memberName(duty.memberId());
            addLine(backingLayout, dutyName,
                    main.isEmpty() ? tr("Nobody assigned yet") : tr("Covering for %1").arg(main));
            ++backing;
        }
    }
    if (serving == 0) {
        auto *none = new QLabel(tr("No duties this Sunday."), this);
        none->setObjectName(QStringLiteral("mutedLabel"));
        servingLayout->addWidget(none);
    }
    layout->addWidget(servingBox);
    if (backing > 0) {
        layout->addWidget(backingBox);
    } else {
        // Not in any layout, so it would sit at the dialog's top-left
        // corner; deleteLater() wouldn't run until the dialog closes (exec()
        // runs its own event loop), so delete it now.
        delete backingBox;
    }

    // --- Buttons -------------------------------------------------------------------
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    QPushButton *allDutiesButton = buttons->addButton(tr("All Duties..."), QDialogButtonBox::ActionRole);
    allDutiesButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(allDutiesButton, &QPushButton::clicked, this, [this]() {
        m_allDutiesRequested = true;
        accept();
    });
    if (canEdit) {
        QPushButton *editButton = buttons->addButton(tr("Edit on Schedule"), QDialogButtonBox::ActionRole);
        connect(editButton, &QPushButton::clicked, this, [this]() {
            m_editRequested = true;
            accept();
        });
    }
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    setMinimumWidth(420);
}
