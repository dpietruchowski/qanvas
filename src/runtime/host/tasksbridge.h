#pragma once

#include <QString>
#include <QVariantList>

#include "runtime/host/storebridge.h"
#include "runtime/storage/tasksstore.h"

class TasksBridge : public StoreBridge
{
    Q_OBJECT

public:
    TasksBridge(const QString& appId, const AppRegistry& registry, QObject* parent = nullptr);

    static QString capability();

    Q_INVOKABLE QVariantList list();
    Q_INVOKABLE bool add(const QString& text);
    Q_INVOKABLE bool update(int id, const QString& text);
    Q_INVOKABLE bool setDone(int id, bool done);
    Q_INVOKABLE bool remove(int id);

private:
    TasksStore m_store;
};
