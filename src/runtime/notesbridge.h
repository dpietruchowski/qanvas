#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include "notesstore.h"

class AppRegistry;

class NotesBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool allowed READ allowed CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    NotesBridge(const QString& appId, const AppRegistry& registry, QObject* parent = nullptr);

    static QString capability();

    bool allowed() const { return m_allowed; }
    int revision() const { return m_revision; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE QVariantList list();
    Q_INVOKABLE bool add(const QString& text);
    Q_INVOKABLE bool update(int id, const QString& text);
    Q_INVOKABLE bool remove(int id);

signals:
    void changed();
    void lastErrorChanged();

private:
    bool permitted();
    bool report(bool ok, const QString& error);
    void setLastError(const QString& error);

    QString m_appId;
    bool m_allowed = false;
    NotesStore m_store;
    int m_revision = 0;
    QString m_lastError;
};
