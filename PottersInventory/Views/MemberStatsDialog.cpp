#include "MemberStatsDialog.h"

#include <algorithm>

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QVBoxLayout>

#include "AvatarLoader.h"
#include "RoleDisplay.h"

MemberStatsDialog::MemberStatsDialog(
    const User &user,
    const QVector<Assignment> &assignments,
    QNetworkAccessManager *networkManager,
    QWidget *parent)
    : FramelessDialog(parent)
{
    setWindowTitle(QStringLiteral("%1 — Responsibilities").arg(user.name()));

    auto *avatar = new QLabel(this);
    if (networkManager) {
        AvatarLoader::loadInto(*networkManager, user.avatarSeed(), avatar, 64);
    }

    auto *nameLabel = new QLabel(user.name(), this);
    nameLabel->setObjectName(QStringLiteral("pageTitle"));
    auto *countLabel = new QLabel(
        QStringLiteral("%1 assignment%2 total").arg(assignments.size()).arg(assignments.size() == 1 ? QString() : QStringLiteral("s")),
        this);
    countLabel->setObjectName(QStringLiteral("pageSubtitle"));

    auto *headerText = new QVBoxLayout;
    headerText->addWidget(nameLabel);
    headerText->addWidget(countLabel);
    auto *header = new QHBoxLayout;
    header->addWidget(avatar);
    header->addSpacing(4);
    header->addLayout(headerText);
    header->addStretch();

    QMap<AssignmentRole, int> counts;
    for (const Assignment &assignment : assignments) {
        counts[assignment.role()]++;
    }
    auto *breakdownLayout = new QHBoxLayout;
    bool anyCounted = false;
    for (AssignmentRole role : allAssignmentRoles()) {
        const int count = counts.value(role, 0);
        if (count == 0) {
            continue;
        }
        anyCounted = true;
        auto *pill = new QLabel(QStringLiteral("%1 %2").arg(RoleDisplay::icon(role)).arg(count), this);
        pill->setToolTip(RoleDisplay::label(role));
        pill->setStyleSheet(QStringLiteral(
            "background: #eef0f4; border-radius: 9px; padding: 3px 10px; font-weight: 600;"));
        breakdownLayout->addWidget(pill);
    }
    if (!anyCounted) {
        breakdownLayout->addWidget(new QLabel(QStringLiteral("No assignments yet."), this));
    }
    breakdownLayout->addStretch();
    auto *breakdownBox = new QGroupBox(QStringLiteral("By Role"), this);
    breakdownBox->setLayout(breakdownLayout);

    QVector<Assignment> sorted = assignments;
    std::sort(sorted.begin(), sorted.end(), [](const Assignment &a, const Assignment &b) {
        return a.serviceDate() > b.serviceDate();
    });
    auto *list = new QListWidget(this);
    for (const Assignment &assignment : sorted) {
        const bool isSupportOnly = assignment.memberId() != user.id() && assignment.supportMemberId() == user.id();
        const QString label = QStringLiteral("%1   %2 %3%4")
            .arg(assignment.serviceDate().toString(QStringLiteral("yyyy-MM-dd")))
            .arg(RoleDisplay::icon(assignment.role()))
            .arg(RoleDisplay::label(assignment.role()))
            .arg(isSupportOnly ? QStringLiteral("  (support)") : QString());
        new QListWidgetItem(label, list);
    }
    auto *listBox = new QGroupBox(QStringLiteral("All Assignments"), this);
    auto *listLayout = new QVBoxLayout;
    listLayout->addWidget(list);
    listBox->setLayout(listLayout);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(header);
    layout->addSpacing(8);
    layout->addWidget(breakdownBox);
    layout->addWidget(listBox, 1);
    layout->addWidget(buttons);

    setMinimumSize(420, 480);
}
