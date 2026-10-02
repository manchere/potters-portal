#include "SettingsView.h"

#include <QButtonGroup>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QStyle>
#include <QVBoxLayout>

SettingsView::SettingsView(QWidget *parent)
    : QWidget(parent)
{
    auto *title = new QLabel(QStringLiteral("Settings"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(QStringLiteral("Choose how the app looks, and change the Admin password."), this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    // --- Appearance -------------------------------------------------------
    auto *themeGroup = new QButtonGroup(this);
    auto *themeRow = new QHBoxLayout;
    themeRow->setSpacing(12);
    const struct
    {
        Theme theme;
        QString description;
        QStringList swatches;
    } themes[] = {
        {Theme::Light, QStringLiteral("White pages with navy buttons."),
         {QStringLiteral("#f7f8fb"), QStringLiteral("#ffffff"), QStringLiteral("#14335c"), QStringLiteral("#d6a537")}},
        {Theme::Black, QStringLiteral("Black pages, easy on the eyes at night."),
         {QStringLiteral("#000000"), QStringLiteral("#0e0e0e"), QStringLiteral("#1f4a85"), QStringLiteral("#d6a537")}},
        {Theme::Navy, QStringLiteral("Deep navy with gold, the colors of the app icon."),
         {QStringLiteral("#0b1a33"), QStringLiteral("#10244a"), QStringLiteral("#d4a72c"), QStringLiteral("#f0c75e")}},
    };
    for (const auto &option : themes) {
        QFrame *card = buildThemeCard(option.theme, option.description, option.swatches);
        themeGroup->addButton(m_themeOptions.last().radio);
        themeRow->addWidget(card, 1);
    }

    auto *appearanceBox = new QGroupBox(QStringLiteral("Appearance"), this);
    auto *appearanceLayout = new QVBoxLayout(appearanceBox);
    appearanceLayout->addLayout(themeRow);

    // --- Admin password ---------------------------------------------------
    m_passwordButton = new QPushButton(QStringLiteral("Change Password..."), this);
    m_passwordButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_passwordButton, &QPushButton::clicked, this, &SettingsView::changePasswordClicked);
    m_passwordHint = new QLabel(this);
    m_passwordHint->setObjectName(QStringLiteral("mutedLabel"));
    m_passwordHint->setWordWrap(true);

    auto *passwordRow = new QHBoxLayout;
    passwordRow->addWidget(m_passwordButton);
    passwordRow->addSpacing(8);
    passwordRow->addWidget(m_passwordHint, 1);
    auto *passwordBox = new QGroupBox(QStringLiteral("Admin password"), this);
    auto *passwordLayout = new QVBoxLayout(passwordBox);
    passwordLayout->addLayout(passwordRow);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(6);
    layout->addWidget(appearanceBox);
    layout->addWidget(passwordBox);
    layout->addStretch();

    setAdminMode(false);
}

QFrame *SettingsView::buildThemeCard(Theme theme, const QString &description, const QStringList &swatches)
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("themeCard"));

    // "&&" so the "&" in "Navy & Gold" isn't taken as a shortcut marker.
    auto *radio = new QRadioButton(themeDisplayName(theme).replace(QLatin1Char('&'), QStringLiteral("&&")), card);
    QFont bold = radio->font();
    bold.setBold(true);
    radio->setFont(bold);
    connect(radio, &QRadioButton::clicked, this, [this, theme]() {
        updateCardHighlight();
        emit themeChosen(theme);
    });

    // A strip of the theme's main colors as a preview. These are fixed, so
    // the dots look the same whichever theme is active.
    auto *swatchRow = new QHBoxLayout;
    swatchRow->setSpacing(4);
    for (const QString &color : swatches) {
        auto *dot = new QLabel(card);
        dot->setFixedSize(22, 22);
        dot->setStyleSheet(QStringLiteral("QLabel { background: %1; border: 1px solid rgba(128,128,128,0.45); "
                                          "border-radius: 11px; }").arg(color));
        swatchRow->addWidget(dot);
    }
    swatchRow->addStretch();

    auto *descriptionLabel = new QLabel(description, card);
    descriptionLabel->setObjectName(QStringLiteral("mutedLabel"));
    descriptionLabel->setWordWrap(true);

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(8);
    cardLayout->addWidget(radio);
    cardLayout->addLayout(swatchRow);
    cardLayout->addWidget(descriptionLabel);

    m_themeOptions.append({theme, card, radio});
    return card;
}

void SettingsView::setCurrentTheme(Theme theme)
{
    for (const ThemeOption &option : std::as_const(m_themeOptions)) {
        if (option.theme == theme) {
            option.radio->setChecked(true);
        }
    }
    updateCardHighlight();
}

void SettingsView::updateCardHighlight()
{
    for (const ThemeOption &option : std::as_const(m_themeOptions)) {
        option.card->setProperty("selected", option.radio->isChecked());
        option.card->style()->unpolish(option.card);
        option.card->style()->polish(option.card);
    }
}

void SettingsView::setAdminMode(bool isAdmin)
{
    m_passwordButton->setEnabled(isAdmin);
    m_passwordHint->setText(isAdmin
        ? QStringLiteral("Sets a new password for the Admin account you're logged in with.")
        : QStringLiteral("Log in as an Admin (the lock at the top right) to change the password."));
}
