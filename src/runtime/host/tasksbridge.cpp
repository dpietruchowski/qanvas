#include "runtime/host/tasksbridge.h"

TasksBridge::TasksBridge(const QString& appId, const AppRegistry& registry, QObject* parent)
    : StoreBridge(appId, capability(), registry, parent)
{
}

QString TasksBridge::capability() { return QStringLiteral("tasks"); }

QVariantList TasksBridge::list()
{
    if (!permitted())
        return {};

    clearError();
    return m_store.list();
}

bool TasksBridge::add(const QString& text)
{
    if (!permitted())
        return false;

    return report(m_store.add(text) > 0, QStringLiteral("A task needs some text"));
}

bool TasksBridge::update(int id, const QString& text)
{
    if (!permitted())
        return false;

    return report(m_store.update(id, text),
                  QStringLiteral("No task %1 to update, or no text for it").arg(id));
}

bool TasksBridge::setDone(int id, bool done)
{
    if (!permitted())
        return false;

    return report(m_store.setDone(id, done),
                  QStringLiteral("No task %1 to mark %2")
                      .arg(id)
                      .arg(done ? QStringLiteral("done") : QStringLiteral("open")));
}

bool TasksBridge::remove(int id)
{
    if (!permitted())
        return false;

    return report(m_store.remove(id), QStringLiteral("No task %1 to remove").arg(id));
}
