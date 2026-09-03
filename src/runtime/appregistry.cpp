#include "appregistry.h"

#include "app.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QJSEngine>
#include <QSqlQuery>
#include <QVariantMap>

namespace
{
QString now() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }
}

AppRegistry::AppRegistry(QObject* parent)
    : QObject(parent)
    , m_database(QStringLiteral("registry"))
{
    m_open = open();
}

AppRegistry& AppRegistry::instance()
{
    static AppRegistry* registry = new AppRegistry(QCoreApplication::instance());
    return *registry;
}

AppRegistry* AppRegistry::create(QQmlEngine*, QJSEngine*)
{
    AppRegistry* registry = &instance();
    QJSEngine::setObjectOwnership(registry, QJSEngine::CppOwnership);
    return registry;
}

bool AppRegistry::open()
{
    if (!m_database.isOpen())
        return false;

    QSqlQuery q(m_database.database());
    return q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS apps ("
                                 "id TEXT PRIMARY KEY, "
                                 "title TEXT NOT NULL, "
                                 "created_at TEXT NOT NULL, "
                                 "updated_at TEXT NOT NULL)"));
}

QVariantList AppRegistry::apps() const
{
    QVariantList apps;
    if (!m_open)
        return apps;

    QSqlQuery q(m_database.database());
    if (!q.exec(QStringLiteral("SELECT id, title, updated_at FROM apps "
                               "ORDER BY updated_at DESC, id ASC")))
        return apps;

    while (q.next())
    {
        apps.append(QVariantMap { { QStringLiteral("id"), q.value(0) },
                                  { QStringLiteral("title"), q.value(1) },
                                  { QStringLiteral("updatedAt"), q.value(2) } });
    }

    return apps;
}

bool AppRegistry::record(const QString& id, const QString& title)
{
    if (!m_open || !App::isValidId(id))
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("INSERT INTO apps (id, title, created_at, updated_at) "
                                  "VALUES (:id, :title, :now, :now) "
                                  "ON CONFLICT(id) DO UPDATE SET title = excluded.title, "
                                  "updated_at = excluded.updated_at")))
        return false;

    const QString name = title.trimmed();
    q.bindValue(QStringLiteral(":id"), id);
    q.bindValue(QStringLiteral(":title"), name.isEmpty() ? id : name);
    q.bindValue(QStringLiteral(":now"), now());

    if (!q.exec())
        return false;

    emit appsChanged();
    return true;
}

bool AppRegistry::remove(const QString& id)
{
    if (!m_open || !App::isValidId(id))
        return false;

    emit appRemoving(id);

    QDir directory(App(id).dir());
    if (directory.exists() && !directory.removeRecursively())
        return false;

    QSqlQuery q(m_database.database());
    if (!q.prepare(QStringLiteral("DELETE FROM apps WHERE id = :id")))
        return false;

    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec())
        return false;

    emit appsChanged();
    return true;
}
