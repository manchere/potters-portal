#include "Language.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

namespace
{
    // Same QSettings location as the theme (see Style.cpp).
    const char *kSettingsOrg = "PottersPortal";
    const char *kSettingsApp = "PottersPortal";
    const char *kLanguageKey = "language";

    QString languageKey(Language language)
    {
        return language == Language::French ? QStringLiteral("fr") : QStringLiteral("en");
    }
}

Language savedLanguage()
{
    const QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    return settings.value(QString::fromLatin1(kLanguageKey)).toString() == languageKey(Language::French)
        ? Language::French
        : Language::English;
}

void saveLanguage(Language language)
{
    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.setValue(QString::fromLatin1(kLanguageKey), languageKey(language));
}

void installLanguage(Language language)
{
    if (language == Language::English) {
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedKingdom));
        return;
    }
    QLocale::setDefault(QLocale(QLocale::French, QLocale::France));

    // The app's own strings, built in from translations/pottersportal_fr.ts.
    auto *appTranslator = new QTranslator(qApp);
    if (appTranslator->load(QStringLiteral(":/i18n/pottersportal_fr.qm"))) {
        QCoreApplication::installTranslator(appTranslator);
    }

    // Qt's own (dialog buttons and the like): windeployqt copies them into
    // translations/ next to the exe; fall back to the Qt install.
    auto *qtTranslator = new QTranslator(qApp);
    const QString deployed = QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
    if (qtTranslator->load(QStringLiteral("qt_fr"), deployed)
        || qtTranslator->load(QStringLiteral("qt_fr"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(qtTranslator);
    }
}
