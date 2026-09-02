#pragma once

#include <QDateTime>
#include <QString>

enum class TransactionType
{
    Purchase,
    Sale,
    Adjustment
};

class Transaction
{
public:
    Transaction() = default;
    Transaction(int id, int itemId, TransactionType type, int quantityChange, const QDateTime &timestamp, QString note = QString());

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int itemId() const { return m_itemId; }
    void setItemId(int itemId) { m_itemId = itemId; }

    TransactionType type() const { return m_type; }
    void setType(TransactionType type) { m_type = type; }

    int quantityChange() const { return m_quantityChange; }
    void setQuantityChange(int quantityChange) { m_quantityChange = quantityChange; }

    QDateTime timestamp() const { return m_timestamp; }
    void setTimestamp(const QDateTime &timestamp) { m_timestamp = timestamp; }

    QString note() const { return m_note; }
    void setNote(const QString &note) { m_note = note; }

private:
    int m_id = -1;
    int m_itemId = -1;
    TransactionType m_type = TransactionType::Adjustment;
    int m_quantityChange = 0;
    QDateTime m_timestamp;
    QString m_note;
};
