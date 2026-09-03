#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>

class QJSEngine;
class QSqlQuery;

// The generated app's whole view of the world, exposed to its QML as "App".
//
// Owns one SQLite file, apps/<id>/app.db. The app may run any SQL against it,
// DDL included — the schema belongs to whoever generated the app, not to the
// runtime. That is safe because the connection is pinned to a single file, so
// isolation is the filesystem's job and no SQL has to be parsed to enforce it.
// Only ATTACH/DETACH would break out of the file, and those are rejected.
//
// Everything handed back to the app is a plain value (QVariant, list, map).
// No QObject* ever crosses into the app, which is what would let the app be
// moved behind a process boundary later without changing its QML.
class AppContext : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    // Bumped by every successful exec(). A signal alone cannot be a binding
    // dependency, so this is what generated QML binds its queries to:
    //     model: (App.revision, App.query("SELECT ..."))
    Q_PROPERTY(int revision READ revision NOTIFY changed)

public:
    AppContext(const QString& id, const QString& dir, QJSEngine* engine = nullptr,
               QObject* parent = nullptr);
    ~AppContext() override;

    QString id() const { return m_id; }
    QString lastError() const { return m_lastError; }
    int revision() const { return m_revision; }

    // Rows come back as a list of maps so the result drops straight into a
    // ListView's model and the app never has to know about Qt's model classes.
    // params is either a list (positional ?) or a map (named :placeholders).
    Q_INVOKABLE QVariantList query(const QString& sql, const QVariant& params = QVariant());

    // For everything that is not a SELECT — DDL, INSERT, UPDATE, DELETE.
    Q_INVOKABLE bool exec(const QString& sql, const QVariant& params = QVariant());

signals:
    void lastErrorChanged();

    // Emitted after every successful exec(). query() returns a snapshot, so
    // generated code binds its models to revision instead of being trusted to
    // call a refresh by hand — which it will forget to do.
    void changed();

private:
    bool open(const QString& dir);
    bool run(QSqlQuery& q, const QString& sql, const QVariant& params);
    void setError(const QString& message);
    void clearError();

    QJSEngine* m_engine = nullptr;
    QString m_id;
    QString m_connectionName;
    QString m_lastError;
    int m_revision = 0;
    bool m_open = false;
};
