#pragma once

#include <QString>
#include <QVariantList>

#include "runtime/storage/systemdatabase.h"

class ChatStore
{
public:
    ChatStore();

    bool isOpen() const { return m_open; }

    QVariantList messages(const QString& app) const;
    bool append(const QString& app, const QString& sender, const QString& text);
    bool rebind(const QString& from, const QString& to);
    bool remove(const QString& app);

private:
    bool open();

    SystemDatabase m_database;
    bool m_open = false;
};
