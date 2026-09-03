#include "runtime/host/storebridge.h"

#include "runtime/app/appregistry.h"

StoreBridge::StoreBridge(const QString& appId, const QString& capability,
                         const AppRegistry& registry, QObject* parent)
    : QObject(parent)
    , m_refusal(QStringLiteral("\"%1\" may not use the user's %2").arg(appId, capability))
    , m_allowed(registry.has(appId, capability))
{
}

bool StoreBridge::permitted()
{
    if (m_allowed)
        return true;

    setLastError(m_refusal);
    return false;
}

bool StoreBridge::report(bool ok, const QString& error)
{
    if (!ok)
    {
        setLastError(error);
        return false;
    }

    clearError();
    ++m_revision;
    emit changed();
    return true;
}

void StoreBridge::clearError() { setLastError(QString()); }

void StoreBridge::setLastError(const QString& error)
{
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}
