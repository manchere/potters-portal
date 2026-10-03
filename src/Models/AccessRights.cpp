#include "AccessRights.h"

bool SectionAccess::allows(AccessAction action) const
{
    switch (action) {
    case AccessAction::View: return view;
    case AccessAction::Create: return create;
    case AccessAction::Update: return update;
    case AccessAction::Delete: return remove;
    }
    return false;
}

void SectionAccess::set(AccessAction action, bool allowed)
{
    switch (action) {
    case AccessAction::View: view = allowed; break;
    case AccessAction::Create: create = allowed; break;
    case AccessAction::Update: update = allowed; break;
    case AccessAction::Delete: remove = allowed; break;
    }
}

SectionAccess &SectionAccess::operator|=(const SectionAccess &other)
{
    view = view || other.view;
    create = create || other.create;
    update = update || other.update;
    remove = remove || other.remove;
    return *this;
}

QVector<Section> allSections()
{
    return {Section::Reports, Section::Songs, Section::Inventory, Section::Taxonomy, Section::Feedback, Section::Settings};
}

QString sectionKey(Section section)
{
    switch (section) {
    case Section::Reports: return QStringLiteral("reports");
    case Section::Songs: return QStringLiteral("songs");
    case Section::Inventory: return QStringLiteral("inventory");
    case Section::Taxonomy: return QStringLiteral("taxonomy");
    case Section::Feedback: return QStringLiteral("feedback");
    case Section::Settings: return QStringLiteral("settings");
    }
    return QString();
}

bool sectionHasAction(Section section, AccessAction action)
{
    if (action == AccessAction::View) {
        return true;
    }
    switch (section) {
    case Section::Settings: return action == AccessAction::Update;
    case Section::Reports: return action == AccessAction::Create;
    default: return true;
    }
}

QString subjectKey(AccessSubject subject)
{
    switch (subject) {
    case AccessSubject::Member: return QStringLiteral("member");
    case AccessSubject::Team: return QStringLiteral("team");
    case AccessSubject::Duty: return QStringLiteral("duty");
    case AccessSubject::Everyone: break;
    }
    return QStringLiteral("everyone");
}

AccessRights AccessRights::full()
{
    AccessRights rights;
    for (Section section : allSections()) {
        rights.grant(section, SectionAccess{true, true, true, true});
    }
    return rights;
}
