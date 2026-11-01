#pragma once

#include <QString>

#include "runtime/storage/systemdatabase.h"

class AppConversations
{
public:
    AppConversations();

    bool isOpen() const { return m_open; }

    QString conversationFor(const QString& appId) const;
    QString appFor(const QString& conversationId) const;
    bool bind(const QString& appId, const QString& conversationId);
    bool remove(const QString& appId);
    bool removeConversation(const QString& conversationId);

private:
    bool open();

    SystemDatabase m_database;
    bool m_open = false;
};
