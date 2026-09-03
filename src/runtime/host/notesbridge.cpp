#include "runtime/host/notesbridge.h"

NotesBridge::NotesBridge(const QString& appId, const AppRegistry& registry, QObject* parent)
    : StoreBridge(appId, capability(), registry, parent)
{
}

QString NotesBridge::capability() { return QStringLiteral("notes"); }

QVariantList NotesBridge::list()
{
    if (!permitted())
        return {};

    clearError();
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
