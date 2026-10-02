#pragma once

#include <QObject>
#include <QVector>

#include "Models/DutyType.h"

// Backed by Postgres (duty_types table). See TagController for the
// query pattern -- DutyType is managed the same way Tags/Categories are.
class DutyTypeController : public QObject
{
    Q_OBJECT

public:
    explicit DutyTypeController(QObject *parent = nullptr);

    QVector<DutyType> allDutyTypes() const;
    DutyType dutyTypeById(int id) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addDutyType(DutyType &dutyType);
    bool updateDutyType(const DutyType &dutyType);
    bool removeDutyType(int id);

signals:
    void dutyTypesChanged();

private:
    mutable QString m_lastError;
};
