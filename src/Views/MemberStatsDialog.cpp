#include "MemberStatsDialog.h"

#include <algorithm>

#include <QLocale>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QVBoxLayout>

#include "Controllers/DutyTypeController.h"
#include "MemberBadge.h"
#include "Models/DutyType.h"

MemberStatsDialog::MemberStatsDialog(
    const User &user,
    const QVector<Duty> &duties,
    DutyTypeController *dutyTypeController,
    QWidget *parent)
    : FramelessDialog(parent)
{
    setWindowTitle(tr("%1 — Responsibilities").arg(user.name()));

    auto *badge = MemberBadge::make(user.name(), user.color(), 64, this);

    auto *nameLabel = new QLabel(user.name(), this);
    nameLabel->setObjectName(QStringLiteral("pageTitle"));
    auto *countLabel = new QLabel(
        tr("%1 %2 total").arg(duties.size()).arg(duties.size() == 1 ? tr("duty") : tr("duties")),
        this);
    countLabel->setObjectName(QStringLiteral("pageSubtitle"));

    auto *headerText = new QVBoxLayout;
    headerText->addWidget(nameLabel);
    headerText->addWidget(countLabel);
    auto *header = new QHBoxLayout;
    header->addWidget(badge);
    header->addSpacing(4);
    header->addLayout(headerText);
    header->addStretch();

    QMap<int, int> counts;
    for (const Duty &duty : duties) {
        counts[duty.dutyTypeId()]++;
    }
    auto *breakdownLayout = new QHBoxLayout;
    bool anyCounted = false;
    for (const DutyType &dutyType : dutyTypeController->allDutyTypes()) {
        const int count = counts.value(dutyType.id(), 0);
        if (count == 0) {
            continue;
        }
        anyCounted = true;
        auto *pill = new QLabel(QStringLiteral("%1 %2").arg(dutyType.icon()).arg(count), this);
        pill->setToolTip(dutyType.name());
        pill->setObjectName(QStringLiteral("statPill"));
        breakdownLayout->addWidget(pill);
    }
    if (!anyCounted) {
        breakdownLayout->addWidget(new QLabel(tr("No duties yet."), this));
    }
    breakdownLayout->addStretch();
    auto *breakdownBox = new QGroupBox(tr("By Duty"), this);
    breakdownBox->setLayout(breakdownLayout);

    QVector<Duty> sorted = duties;
    std::sort(sorted.begin(), sorted.end(), [](const Duty &a, const Duty &b) {
        return a.serviceDate() > b.serviceDate();
    });
    auto *list = new QListWidget(this);
    for (const Duty &duty : sorted) {
        const bool isSupportOnly = duty.memberId() != user.id() && duty.supportMemberId() == user.id();
        const DutyType dutyType = dutyTypeController->dutyTypeById(duty.dutyTypeId());
        const QString label = QStringLiteral("%1   %2 %3%4")
            .arg(QLocale().toString(duty.serviceDate(), QStringLiteral("yyyy-MM-dd")))
            .arg(dutyType.icon())
            .arg(dutyType.name())
            .arg(isSupportOnly ? tr("  (support)") : QString());
        new QListWidgetItem(label, list);
    }
    auto *listBox = new QGroupBox(tr("All Duties"), this);
    auto *listLayout = new QVBoxLayout;
    listLayout->addWidget(list);
    listBox->setLayout(listLayout);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = contentLayout();
    layout->addLayout(header);
    layout->addSpacing(8);
    layout->addWidget(breakdownBox);
    layout->addWidget(listBox, 1);
    layout->addWidget(buttons);

    setMinimumSize(420, 480);
}
