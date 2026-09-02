#include "ReportController.h"

ReportController::ReportController(QObject *parent)
    : QObject(parent)
{
}

QVector<Transaction> ReportController::allTransactions() const
{
    return m_transactions;
}

QVector<Transaction> ReportController::transactionsForItem(int itemId) const
{
    QVector<Transaction> result;
    for (const Transaction &transaction : m_transactions) {
        if (transaction.itemId() == itemId) {
            result.append(transaction);
        }
    }
    return result;
}

void ReportController::recordTransaction(Transaction transaction)
{
    transaction.setId(m_nextId++);
    m_transactions.append(transaction);
    emit transactionsChanged();
}
