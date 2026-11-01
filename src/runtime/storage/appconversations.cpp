#include "runtime/storage/appconversations.h"

#include <QSqlQuery>

AppConversations::AppConversations()
    : m_database(QStringLiteral("app_conversations"))
{
    m_open = open();
}

bool AppConversations::open()
{
    if (!m_database.isOpen())
        return false;

    QSqlQuery q(m_database.database());
    return q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS app_conversation ("
                                 "app_id TEXT PRIMARY KEY, "
                                 "conversation_id TEXT NOT NULL)"));
}

QString AppConversations::conversationFor(const QString& appId) const
{
    if (!m_open)
        return QString();

    QSqlQuery q(m_database.database());
    if (!q.prepare(
            QStringLiteral("SELECT conversation_id FROM app_conversation WHERE app_id = :app")))
        return QString();

    q.bindValue(QStringLiteral(":app"), appId);
    if (!q.exec() || !q.next())
        return QString();

    return q.value(0).toString();
}

QString AppConversations::appFor(const QString& conversationId) const
{
    if (!m_open)
        return QString();

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("SELECT app_id FROM app_conversation "
                                  "WHERE conversation_id = :conversation "
                                  "ORDER BY rowid DESC LIMIT 1")))
        return QString();

    q.bindValue(QStringLiteral(":conversation"), conversationId);
    if (!q.exec() || !q.next())
        return QString();

    return q.value(0).toString();
}

bool AppConversations::bind(const QString& appId, const QString& conversationId)
{
    if (!m_open)
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("INSERT OR REPLACE INTO app_conversation "
                                  "(app_id, conversation_id) VALUES (:app, :conversation)")))
        return false;

    q.bindValue(QStringLiteral(":app"), appId);
    q.bindValue(QStringLiteral(":conversation"), conversationId);
    return q.exec();
}

bool AppConversations::remove(const QString& appId)
{
    if (!m_open)
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("DELETE FROM app_conversation WHERE app_id = :app")))
        return false;

    q.bindValue(QStringLiteral(":app"), appId);
    return q.exec();
}

bool AppConversations::removeConversation(const QString& conversationId)
{
    if (!m_open)
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(
            QStringLiteral("DELETE FROM app_conversation WHERE conversation_id = :conversation")))
        return false;

    q.bindValue(QStringLiteral(":conversation"), conversationId);
    return q.exec();
}
