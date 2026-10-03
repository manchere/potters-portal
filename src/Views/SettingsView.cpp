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
    auto *title = new QLabel(tr("Settings"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    m_subtitle = new QLabel(this);
    m_subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    m_subtitle->setWordWrap(true);

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
        {Theme::Light, tr("White pages with navy buttons."),
         {QStringLiteral("#f7f8fb"), QStringLiteral("#ffffff"), QStringLiteral("#14335c"), QStringLiteral("#d6a537")}},
        {Theme::Black, tr("Black pages, easy on the eyes at night."),
         {QStringLiteral("#000000"), QStringLiteral("#0e0e0e"), QStringLiteral("#1f4a85"), QStringLiteral("#d6a537")}},
        {Theme::Navy, tr("Deep navy with gold, the colors of the app icon."),
         {QStringLiteral("#0b1a33"), QStringLiteral("#10244a"), QStringLiteral("#d4a72c"), QStringLiteral("#f0c75e")}},
    };
    for (const auto &option : themes) {
        QFrame *card = buildThemeCard(option.theme, option.description, option.swatches);
        themeGroup->addButton(m_themeOptions.last().radio);
        themeRow->addWidget(card, 1);
    }

    m_themeLockedHint = new QLabel(
        tr("Ask an Admin for access to change the theme colors."), this);
    m_themeLockedHint->setObjectName(QStringLiteral("mutedLabel"));
    m_themeLockedHint->setWordWrap(true);

    auto *appearanceBox = new QGroupBox(tr("Appearance"), this);
    auto *appearanceLayout = new QVBoxLayout(appearanceBox);
    appearanceLayout->addLayout(themeRow);
    appearanceLayout->addWidget(m_themeLockedHint);

    // --- Language -----------------------------------------------------------
    // Each name in its own language, so it can be found whichever is active.
    m_englishRadio = new QRadioButton(QStringLiteral("English (ENG)"), this);
    m_frenchRadio = new QRadioButton(QStringLiteral("Français (FR)"), this);
    auto *languageGroup = new QButtonGroup(this);
    languageGroup->addButton(m_englishRadio);
    languageGroup->addButton(m_frenchRadio);
    connect(m_englishRadio, &QRadioButton::clicked, this, [this]() { emit languageChosen(Language::English); });
    connect(m_frenchRadio, &QRadioButton::clicked, this, [this]() { emit languageChosen(Language::French); });
    auto *languageHint = new QLabel(tr("The app restarts to switch language."), this);
    languageHint->setObjectName(QStringLiteral("mutedLabel"));

    auto *languageRow = new QHBoxLayout;
    languageRow->addWidget(m_englishRadio);
    languageRow->addSpacing(16);
    languageRow->addWidget(m_frenchRadio);
    languageRow->addSpacing(16);
    languageRow->addWidget(languageHint, 1);
    auto *languageBox = new QGroupBox(tr("Language"), this);
    auto *languageLayout = new QVBoxLayout(languageBox);
    languageLayout->addLayout(languageRow);

    // --- Admin password ---------------------------------------------------
    auto *passwordButton = new QPushButton(tr("Change Password..."), this);
    passwordButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(passwordButton, &QPushButton::clicked, this, &SettingsView::changePasswordClicked);
    auto *passwordHint = new QLabel(
        tr("Sets a new password for the Admin account you're logged in with."), this);
    passwordHint->setObjectName(QStringLiteral("mutedLabel"));
    passwordHint->setWordWrap(true);

    auto *passwordRow = new QHBoxLayout;
    passwordRow->addWidget(passwordButton);
    passwordRow->addSpacing(8);
    passwordRow->addWidget(passwordHint, 1);
    m_passwordBox = new QGroupBox(tr("Admin password"), this);
    auto *passwordLayout = new QVBoxLayout(m_passwordBox);
    passwordLayout->addLayout(passwordRow);

    // --- Access rights (Admin) ------------------------------------------------
    auto *accessButton = new QPushButton(tr("Manage Access Rights..."), this);
    accessButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(accessButton, &QPushButton::clicked, this, &SettingsView::accessRightsClicked);
    auto *accessHint = new QLabel(
        tr("Choose what everyone, each member, team or duty can open and change in Reports, Songs, "
           "Inventory, Taxonomy, Feedback and Settings."),
        this);
    accessHint->setObjectName(QStringLiteral("mutedLabel"));
    accessHint->setWordWrap(true);
    auto *accessRow = new QHBoxLayout;
    accessRow->addWidget(accessButton);
    accessRow->addSpacing(8);
    accessRow->addWidget(accessHint, 1);
    m_accessBox = new QGroupBox(tr("Access rights"), this);
    auto *accessLayout = new QVBoxLayout(m_accessBox);
    accessLayout->addLayout(accessRow);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(m_subtitle);
    layout->addSpacing(6);
    layout->addWidget(appearanceBox);
    layout->addWidget(languageBox);
    layout->addWidget(m_passwordBox);
    layout->addWidget(m_accessBox);
    layout->addStretch();

    setAdminMode(false);
    setCanChangeTheme(false);
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

void SettingsView::setCurrentLanguage(Language language)
{
    (language == Language::French ? m_frenchRadio : m_englishRadio)->setChecked(true);
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
    m_passwordBox->setVisible(isAdmin);
    m_accessBox->setVisible(isAdmin);
    m_subtitle->setText(isAdmin
        ? tr("Choose how the app looks and its language, change the Admin password, and set who can do what.")
        : tr("Choose how the app looks and its language."));
}

void SettingsView::setCanChangeTheme(bool canChange)
{
    for (const ThemeOption &option : std::as_const(m_themeOptions)) {
        option.radio->setEnabled(canChange);
    }
    m_themeLockedHint->setVisible(!canChange);
}
