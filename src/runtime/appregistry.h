#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;

// The runtime's own record of the apps it wrote — system.db next to the app
// directories. Apps never reach it: they only ever see their own app.db.
class AppRegistry : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)

public:
    // No default argument: a default-constructible singleton makes the QML
    // engine build its own instance instead of calling create().
    explicit AppRegistry(QObject* parent);
    ~AppRegistry() override;

    static AppRegistry& instance();
    static AppRegistry* create(QQmlEngine* engine, QJSEngine* scriptEngine);

    QVariantList apps() const;
    bool record(const QString& id, const QString& title);

signals:
    void appsChanged();

private:
    bool open();

    QString m_connectionName;
    bool m_open = false;
};
