#pragma once

#include <QString>
#include <QVariantList>

#include "systemdatabase.h"

class NotesStore
{
public:
    NotesStore();

    bool isOpen() const { return m_open; }

    QVariantList list() const;
    qint64 add(const QString& text);
    bool update(qint64 id, const QString& text);
    bool remove(qint64 id);

private:
    bool open();

    SystemDatabase m_database;
    bool m_open = false;
};
