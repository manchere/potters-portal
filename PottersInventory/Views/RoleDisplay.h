#pragma once

#include <QString>

#include "Models/Assignment.h"

// Presentation mapping for AssignmentRole -- kept out of Models/Assignment
// (shared, UI-toolkit-agnostic) since an icon glyph is desktop UI concern,
// not something the REST server needs.
namespace RoleDisplay
{
    // A distinct emoji glyph per role -- no icon asset files/resource
    // compilation needed, and Qt6 on Windows renders color emoji natively.
    QString icon(AssignmentRole role);

    // Capitalized display label, e.g. "Singing".
    QString label(AssignmentRole role);

    // icon() + label() combined, e.g. "🎤 Singing" -- used in combo boxes.
    QString iconAndLabel(AssignmentRole role);
}
