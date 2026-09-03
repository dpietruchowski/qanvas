#pragma once

#include <QObject>
#include <QString>

class AppRegistry;

class StoreBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool allowed READ allowed CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    StoreBridge(const QString& appId, const QString& capability, const AppRegistry& registry,
                QObject* parent = nullptr);

    bool allowed() const { return m_allowed; }
    int revision() const { return m_revision; }
    QString lastError() const { return m_lastError; }

signals:
    void changed();
    void lastErrorChanged();

protected:
    bool permitted();
    bool report(bool ok, const QString& error);
    void clearError();

private:
    void setLastError(const QString& error);

    QString m_refusal;
    bool m_allowed = false;
    int m_revision = 0;
    QString m_lastError;
};
