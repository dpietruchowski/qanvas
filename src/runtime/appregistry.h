#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

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
    ~AppRegistry() override;

    static AppRegistry& instance();
    static AppRegistry* create(QQmlEngine* engine, QJSEngine* scriptEngine);

    QVariantList apps() const;
    bool record(const QString& id, const QString& title);
    Q_INVOKABLE bool remove(const QString& id);

signals:
    void appsChanged();
    void appRemoving(const QString& id);

private:
    bool open();

    QString m_connectionName;
    bool m_open = false;
};
