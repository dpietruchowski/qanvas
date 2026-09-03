#include "runtime/app/appschema.h"

#include "runtime/app/app.h"

#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>

namespace
{
QString nextConnectionName()
{
    static int serial = 0;
    return QStringLiteral("qanvas_schema_%1").arg(++serial);
}

QString quoted(const QString& name)
{
    return QLatin1Char('"') + QString(name).replace(QLatin1Char('"'), QStringLiteral("\"\""))
        + QLatin1Char('"');
}

QString rows(qint64 count)
{
    return count == 1 ? QStringLiteral("1 row") : QStringLiteral("%1 rows").arg(count);
}
}

AppSchema::AppSchema(const App& app)
{
    const QString path = app.databasePath();
    if (path.isEmpty() || !QFileInfo::exists(path))
        return;

    m_connectionName = nextConnectionName();
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(path);
    db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    m_open = db.open();
}

AppSchema::~AppSchema()
{
    if (m_connectionName.isEmpty() || !QSqlDatabase::contains(m_connectionName))
        return;

    QSqlDatabase::database(m_connectionName, false).close();
    QSqlDatabase::removeDatabase(m_connectionName);
}

QString AppSchema::describe() const
{
    if (!m_open)
        return QStringLiteral("No database yet");

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("SELECT type, name, sql FROM sqlite_master "
                               "WHERE sql IS NOT NULL AND name NOT LIKE 'sqlite_%' "
                               "ORDER BY CASE type WHEN 'table' THEN 0 ELSE 1 END, name")))
        return QStringLiteral("Could not read the database");

    QStringList entries;
    while (q.next())
    {
        const QString type = q.value(0).toString();
        const QString name = q.value(1).toString();
        const QString sql = q.value(2).toString().trimmed();

        if (type != QStringLiteral("table"))
        {
            entries.append(sql);
            continue;
        }

        QSqlQuery count(db);
        qint64 total = 0;
        if (count.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(quoted(name))) && count.next())
            total = count.value(0).toLongLong();
        entries.append(QStringLiteral("%1 — %2\n%3").arg(name, rows(total), sql));
    }

    if (entries.isEmpty())
        return QStringLiteral("The database has no tables yet");
    return entries.join(QStringLiteral("\n\n"));
}
