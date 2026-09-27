#pragma once

#include <QObject>
#include <QVector>

#include "Models/RoleType.h"

// Backed by Postgres (assignment_roles table). See TagController for the
// query pattern -- RoleType is managed the same way Tags/Categories are.
class RoleTypeController : public QObject
{
    Q_OBJECT

public:
    explicit RoleTypeController(QObject *parent = nullptr);

    QVector<RoleType> allRoleTypes() const;
    RoleType roleTypeById(int id) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addRoleType(RoleType &roleType);
    bool updateRoleType(const RoleType &roleType);
    bool removeRoleType(int id);

signals:
    void roleTypesChanged();

private:
    mutable QString m_lastError;
};
