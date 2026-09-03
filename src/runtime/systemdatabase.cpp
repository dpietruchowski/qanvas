#include "systemdatabase.h"

#include "app.h"

#include <QDir>

namespace
{
QString nextConnectionName(const QString& purpose)
{
    static int serial = 0;
    return QStringLiteral("qanvas_%1_%2").arg(purpose).arg(++serial);
}
}

SystemDatabase::SystemDatabase(const QString& purpose)
    : m_connectionName(nextConnectionName(purpose))
{
    const QString root = App::root();
    if (!QDir().mkpath(root))
        return;

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(root + QStringLiteral("/system.db"));
    m_open = db.open();
}

SystemDatabase::~SystemDatabase()
{
    if (!QSqlDatabase::contains(m_connectionName))
        return;

    QSqlDatabase::database(m_connectionName, false).close();
    QSqlDatabase::removeDatabase(m_connectionName);
}

QSqlDatabase SystemDatabase::database() const
{
    return QSqlDatabase::database(m_connectionName, false);
}
