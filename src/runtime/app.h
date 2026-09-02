#pragma once

#include <QString>

class App
{
public:
    static QString root();
    static bool isValidId(const QString& id);

    explicit App(QString id);

    QString id() const { return m_id; }
    bool isValid() const;
    bool exists() const;

    QString dir() const;
    QString entryPath() const;
    QString databasePath() const;
    QString iconPath() const;

private:
    QString child(const QString& name) const;

    QString m_id;
};
