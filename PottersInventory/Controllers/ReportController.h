#pragma once

#include <QObject>
#include <QVector>

#include "Models/Transaction.h"

class ReportController : public QObject
{
    Q_OBJECT

public:
    explicit ReportController(QObject *parent = nullptr);

    QVector<Transaction> allTransactions() const;
    QVector<Transaction> transactionsForItem(int itemId) const;

public slots:
    void recordTransaction(Transaction transaction);

signals:
    void transactionsChanged();

private:
    QVector<Transaction> m_transactions;
    int m_nextId = 1;
};
