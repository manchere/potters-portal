#pragma once

#include <QString>

// The app's display language, picked on the Settings page and kept in
// QSettings next to the theme. English is the source language; French
// comes from translations/pottersportal_fr.ts (built into the app), plus
// Qt's own French strings for standard buttons such as Yes/No/Cancel.
// A change takes effect on the next launch -- the Settings page offers to
// restart right away.
enum class Language
{
    English,
    French,
};

// The language last picked (English until changed).
Language savedLanguage();
void saveLanguage(Language language);

// Loads the translations for language and makes dates use its month and
// day names. Call once at startup, before any window is created.
void installLanguage(Language language);
