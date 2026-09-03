#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "runtime/storage/systemdatabase.h"

class QJSEngine;
class QQmlEngine;

class AppRegistry : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)

public:
    explicit AppRegistry(QObject* parent);

    static AppRegistry& instance();
    static AppRegistry* create(QQmlEngine* engine, QJSEngine* scriptEngine);

    QVariantList apps() const;
    bool record(const QString& id, const QString& title);
    Q_INVOKABLE bool remove(const QString& id);

    QStringList capabilities(const QString& id) const;
    bool has(const QString& id, const QString& capability) const;
    bool grant(const QString& id, const QString& capability);

signals:
    void appsChanged();
    void appRemoving(const QString& id);

private:
    bool open();

    SystemDatabase m_database;
    bool m_open = false;
};
