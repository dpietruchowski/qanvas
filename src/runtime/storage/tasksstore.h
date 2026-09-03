#pragma once

#include <QString>
#include <QVariantList>

#include "runtime/storage/systemdatabase.h"

class TasksStore
{
public:
    TasksStore();

    bool isOpen() const { return m_open; }

    QVariantList list() const;
    qint64 add(const QString& text);
    bool update(qint64 id, const QString& text);
    bool setDone(qint64 id, bool done);
    bool remove(qint64 id);

private:
    bool open();
    bool touch(const QString& sql, qint64 id, const QVariant& value);

    SystemDatabase m_database;
    bool m_open = false;
};
