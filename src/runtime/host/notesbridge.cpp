#include "runtime/host/notesbridge.h"

#include "runtime/app/appregistry.h"

NotesBridge::NotesBridge(const QString& appId, const AppRegistry& registry, QObject* parent)
    : QObject(parent)
    , m_appId(appId)
    , m_allowed(registry.has(appId, capability()))
{
}

QString NotesBridge::capability() { return QStringLiteral("notes"); }

bool NotesBridge::permitted()
{
    if (m_allowed)
        return true;

    setLastError(QStringLiteral("\"%1\" may not use the user's notes").arg(m_appId));
    return false;
}

bool NotesBridge::report(bool ok, const QString& error)
{
    if (!ok)
    {
        setLastError(error);
        return false;
    }

    setLastError(QString());
    ++m_revision;
    emit changed();
    return true;
}

void NotesBridge::setLastError(const QString& error)
{
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}

QVariantList NotesBridge::list()
{
    if (!permitted())
        return {};

    setLastError(QString());
    return m_store.list();
}

bool NotesBridge::add(const QString& text)
{
    if (!permitted())
        return false;

    return report(m_store.add(text) > 0, QStringLiteral("A note needs some text"));
}

bool NotesBridge::update(int id, const QString& text)
{
    if (!permitted())
        return false;

    return report(m_store.update(id, text),
                  QStringLiteral("No note %1 to update, or no text for it").arg(id));
}

bool NotesBridge::remove(int id)
{
    if (!permitted())
        return false;

    return report(m_store.remove(id), QStringLiteral("No note %1 to remove").arg(id));
}
