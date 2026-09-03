#pragma once

#include <QSqlDatabase>
#include <QString>

class SystemDatabase
{
public:
    explicit SystemDatabase(const QString& purpose);
    ~SystemDatabase();

    SystemDatabase(const SystemDatabase&) = delete;
    SystemDatabase& operator=(const SystemDatabase&) = delete;

    bool isOpen() const { return m_open; }
    QSqlDatabase database() const;

private:
    QString m_connectionName;
    bool m_open = false;
};
