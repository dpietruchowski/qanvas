#include "runtime/host/appcontext.h"

#include <QDir>
#include <QJSEngine>
#include <QJSValue>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariantMap>

namespace
{

QString nextConnectionName(const QString& id)
{
    static int serial = 0;
    return QStringLiteral("qanvas_app_%1_%2").arg(id).arg(++serial);
}

// Reaching another app's data means going through the runtime, never through
// SQL. These are the only two statements that can leave the app's own file.
bool escapesTheFile(const QString& sql)
{
    static const QRegularExpression re(QStringLiteral("\\b(attach|detach)\\b"),
                                       QRegularExpression::CaseInsensitiveOption);
    return re.match(sql).hasMatch();
}

QVariant bindable(const QVariant& value)
{
    if (value.typeId() == QMetaType::QString && value.toString().isNull())
        return QStringLiteral("");
    return value;
}

QVariant plain(const QVariant& params)
{
    if (params.metaType() == QMetaType::fromType<QJSValue>())
        return params.value<QJSValue>().toVariant();
    return params;
}

void bindParams(QSqlQuery& q, const QVariant& rawParams)
{
    const QVariant params = plain(rawParams);
    if (params.typeId() == QMetaType::QVariantMap)
    {
        const QVariantMap map = params.toMap();
        for (auto it = map.cbegin(); it != map.cend(); ++it)
        {
            const QString key
                = it.key().startsWith(QLatin1Char(':')) ? it.key() : QStringLiteral(":") + it.key();
            q.bindValue(key, bindable(it.value()));
        }
        return;
    }

    const QVariantList list = params.toList();
    for (const QVariant& value : list)
        q.addBindValue(bindable(value));
}

}  // namespace

AppContext::AppContext(const QString& id, const QString& dir, QJSEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
    , m_id(id)
    , m_connectionName(nextConnectionName(id))
{
    m_open = open(dir);
}

AppContext::~AppContext()
{
    // The QSqlDatabase handle is never held as a member: an outstanding copy
    // makes removeDatabase() warn and leaks the connection.
    if (QSqlDatabase::contains(m_connectionName))
    {
        QSqlDatabase::database(m_connectionName, false).close();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool AppContext::open(const QString& dir)
{
    if (!QDir().mkpath(dir))
    {
        setError(QStringLiteral("Could not create app directory: %1").arg(dir));
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(dir + QStringLiteral("/app.db"));
    if (!db.open())
    {
        setError(db.lastError().text());
        return false;
    }

    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    return true;
}

bool AppContext::run(QSqlQuery& q, const QString& sql, const QVariant& params)
{
    if (!m_open)
    {
        setError(QStringLiteral("Database is not open"));
        return false;
    }
    if (escapesTheFile(sql))
    {
        setError(QStringLiteral("ATTACH and DETACH are not allowed"));
        return false;
    }
    if (!q.prepare(sql))
    {
        setError(q.lastError().text());
        return false;
    }

    bindParams(q, params);

    if (!q.exec())
    {
        setError(q.lastError().text());
        return false;
    }

    clearError();
    return true;
}

QVariantList AppContext::query(const QString& sql, const QVariant& params)
{
    QVariantList rows;

    QSqlQuery q(QSqlDatabase::database(m_connectionName, false));
    if (!run(q, sql, params))
        return rows;

    const QSqlRecord record = q.record();
    const int columns = record.count();

    while (q.next())
    {
        QVariantMap row;
        for (int i = 0; i < columns; ++i)
        {
            const QVariant value = q.value(i);
            row.insert(record.fieldName(i), value.isNull() ? QVariant() : value);
        }
        rows.append(row);
    }

    return rows;
}

bool AppContext::exec(const QString& sql, const QVariant& params)
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName, false));
    if (!run(q, sql, params))
        return false;

    ++m_revision;
    emit changed();
    return true;
}

void AppContext::setError(const QString& message)
{
    if (!message.isEmpty() && m_engine)
        m_engine->throwError(message);

    if (m_lastError == message)
        return;
    m_lastError = message;
    emit lastErrorChanged();
}

void AppContext::clearError() { setError(QString()); }
