#include "chatstore.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QVariantMap>

namespace
{
QString now() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }

QVariant key(const QString& app) { return app.isEmpty() ? QString::fromLatin1("") : app; }
}

ChatStore::ChatStore()
    : m_database(QStringLiteral("chat"))
{
    m_open = open();
}

bool ChatStore::open()
{
    if (!m_database.isOpen())
        return false;

    QSqlQuery q(m_database.database());
    if (!q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS chat_messages ("
                               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                               "app TEXT NOT NULL, "
                               "sender TEXT NOT NULL, "
                               "text TEXT NOT NULL, "
                               "created_at TEXT NOT NULL)")))
        return false;

    return q.exec(
        QStringLiteral("CREATE INDEX IF NOT EXISTS chat_messages_app ON chat_messages (app, id)"));
}

QVariantList ChatStore::messages(const QString& app) const
{
    QVariantList messages;
    if (!m_open)
        return messages;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral(
            "SELECT sender, text FROM chat_messages WHERE app = :app ORDER BY id ASC")))
        return messages;

    q.bindValue(QStringLiteral(":app"), key(app));
    if (!q.exec())
        return messages;

    while (q.next())
    {
        messages.append(QVariantMap { { QStringLiteral("sender"), q.value(0) },
                                      { QStringLiteral("text"), q.value(1) } });
    }
    return messages;
}

bool ChatStore::append(const QString& app, const QString& sender, const QString& text)
{
    if (!m_open)
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("INSERT INTO chat_messages (app, sender, text, created_at) "
                                  "VALUES (:app, :sender, :text, :now)")))
        return false;

    q.bindValue(QStringLiteral(":app"), key(app));
    q.bindValue(QStringLiteral(":sender"), sender);
    q.bindValue(QStringLiteral(":text"), text);
    q.bindValue(QStringLiteral(":now"), now());
    return q.exec();
}

bool ChatStore::rebind(const QString& from, const QString& to)
{
    if (!m_open)
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("UPDATE chat_messages SET app = :to WHERE app = :from")))
        return false;

    q.bindValue(QStringLiteral(":to"), key(to));
    q.bindValue(QStringLiteral(":from"), key(from));
    return q.exec();
}

bool ChatStore::remove(const QString& app)
{
    if (!m_open)
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("DELETE FROM chat_messages WHERE app = :app")))
        return false;

    q.bindValue(QStringLiteral(":app"), key(app));
    return q.exec();
}
