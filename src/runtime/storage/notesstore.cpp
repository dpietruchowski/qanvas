#include "runtime/storage/notesstore.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QVariantMap>

namespace
{
QString now() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }
}

NotesStore::NotesStore()
    : m_database(QStringLiteral("notes"))
{
    m_open = open();
}

bool NotesStore::open()
{
    if (!m_database.isOpen())
        return false;

    QSqlQuery q(m_database.database());
    return q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS notes ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "text TEXT NOT NULL, "
                                 "created_at TEXT NOT NULL, "
                                 "updated_at TEXT NOT NULL)"));
}

QVariantList NotesStore::list() const
{
    QVariantList notes;
    if (!m_open)
        return notes;

    QSqlQuery q(m_database.database());
    if (!q.exec(QStringLiteral("SELECT id, text, created_at, updated_at FROM notes "
                               "ORDER BY updated_at DESC, id DESC")))
        return notes;

    while (q.next())
    {
        notes.append(QVariantMap { { QStringLiteral("id"), q.value(0) },
                                   { QStringLiteral("text"), q.value(1) },
                                   { QStringLiteral("createdAt"), q.value(2) },
                                   { QStringLiteral("updatedAt"), q.value(3) } });
    }
    return notes;
}

qint64 NotesStore::add(const QString& text)
{
    const QString body = text.trimmed();
    if (!m_open || body.isEmpty())
        return -1;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("INSERT INTO notes (text, created_at, updated_at) "
                                  "VALUES (:text, :now, :now)")))
        return -1;

    q.bindValue(QStringLiteral(":text"), body);
    q.bindValue(QStringLiteral(":now"), now());
    if (!q.exec())
        return -1;

    return q.lastInsertId().toLongLong();
}

bool NotesStore::update(qint64 id, const QString& text)
{
    const QString body = text.trimmed();
    if (!m_open || body.isEmpty())
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(
            QStringLiteral("UPDATE notes SET text = :text, updated_at = :now WHERE id = :id")))
        return false;

    q.bindValue(QStringLiteral(":text"), body);
    q.bindValue(QStringLiteral(":now"), now());
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec() && q.numRowsAffected() == 1;
}

bool NotesStore::remove(qint64 id)
{
    if (!m_open)
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("DELETE FROM notes WHERE id = :id")))
        return false;

    q.bindValue(QStringLiteral(":id"), id);
    return q.exec() && q.numRowsAffected() == 1;
}
