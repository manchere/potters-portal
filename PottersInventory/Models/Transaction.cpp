#include "Transaction.h"

Transaction::Transaction(int id, int itemId, TransactionType type, int quantityChange, const QDateTime &timestamp, QString note)
    : m_id(id)
    , m_itemId(itemId)
    , m_type(type)
    , m_quantityChange(quantityChange)
    , m_timestamp(timestamp)
    , m_note(std::move(note))
{
}
