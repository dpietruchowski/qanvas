#include "runtime/storage/tasksstore.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QVariantMap>

namespace
{
QString now() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }
}

TasksStore::TasksStore()
    : m_database(QStringLiteral("tasks"))
{
    m_open = open();
}

bool TasksStore::open()
{
    if (!m_database.isOpen())
        return false;

    QSqlQuery q(m_database.database());
    return q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS tasks ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "text TEXT NOT NULL, "
                                 "done INTEGER NOT NULL DEFAULT 0, "
                                 "created_at TEXT NOT NULL, "
                                 "updated_at TEXT NOT NULL)"));
}

QVariantList TasksStore::list() const
{
    QVariantList tasks;
    if (!m_open)
        return tasks;

    QSqlQuery q(m_database.database());
    if (!q.exec(QStringLiteral("SELECT id, text, done, created_at, updated_at FROM tasks "
                               "ORDER BY done ASC, updated_at DESC, id DESC")))
        return tasks;

    while (q.next())
    {
        tasks.append(QVariantMap { { QStringLiteral("id"), q.value(0) },
                                   { QStringLiteral("text"), q.value(1) },
                                   { QStringLiteral("done"), q.value(2).toBool() },
                                   { QStringLiteral("createdAt"), q.value(3) },
                                   { QStringLiteral("updatedAt"), q.value(4) } });
    }
    return tasks;
}

qint64 TasksStore::add(const QString& text)
{
    const QString body = text.trimmed();
    if (!m_open || body.isEmpty())
        return -1;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("INSERT INTO tasks (text, created_at, updated_at) "
                                  "VALUES (:text, :now, :now)")))
        return -1;

    q.bindValue(QStringLiteral(":text"), body);
    q.bindValue(QStringLiteral(":now"), now());
    if (!q.exec())
        return -1;

    return q.lastInsertId().toLongLong();
}

bool TasksStore::update(qint64 id, const QString& text)
{
    const QString body = text.trimmed();
    if (body.isEmpty())
        return false;

    return touch(QStringLiteral("UPDATE tasks SET text = :value, updated_at = :now WHERE id = :id"),
                 id, body);
}

bool TasksStore::setDone(qint64 id, bool done)
{
    return touch(QStringLiteral("UPDATE tasks SET done = :value, updated_at = :now WHERE id = :id"),
                 id, done ? 1 : 0);
}

bool TasksStore::touch(const QString& sql, qint64 id, const QVariant& value)
{
    if (!m_open)
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(sql))
        return false;

    q.bindValue(QStringLiteral(":value"), value);
    q.bindValue(QStringLiteral(":now"), now());
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec() && q.numRowsAffected() == 1;
}

bool TasksStore::remove(qint64 id)
{
    if (!m_open)
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("DELETE FROM tasks WHERE id = :id")))
        return false;

    q.bindValue(QStringLiteral(":id"), id);
    return q.exec() && q.numRowsAffected() == 1;
}
