#pragma once

#include <memory>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "agent/agentcontroller.h"
#include "agent/client.h"
#include "utils/securestorage.h"

class ChatAgent : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QStringList providers READ providers CONSTANT)
    Q_PROPERTY(QString provider READ provider WRITE setProvider NOTIFY providerChanged)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString reasoning READ reasoning WRITE setReasoning NOTIFY reasoningChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString apiKeyHint READ apiKeyHint NOTIFY apiKeyHintChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastAppId READ lastAppId NOTIFY lastAppIdChanged)

public:
    explicit ChatAgent(QObject* parent = nullptr);

    QStringList providers() const;
    QString provider() const { return m_provider; }
    QString model() const { return m_model; }
    QString reasoning() const { return m_reasoning; }
    QString apiKey() const { return m_apiKey; }
    QString apiKeyHint() const;
    QVariantList messages() const;
    bool busy() const { return m_controller.isBusy(); }
    QString lastAppId() const { return m_lastAppId; }

    void setProvider(const QString& provider);
    void setModel(const QString& model);
    void setReasoning(const QString& reasoning);
    void setApiKey(const QString& apiKey);

    Q_INVOKABLE void send(const QString& text);
    Q_INVOKABLE void clear();

signals:
    void providerChanged();
    void modelChanged();
    void reasoningChanged();
    void apiKeyChanged();
    void apiKeyHintChanged();
    void messagesChanged();
    void busyChanged();
    void lastAppIdChanged();
    void appCreated(const QString& id);

private:
    void registerTools();
    void load();
    void store(const QString& key, const QString& value);
    QString effectiveApiKey() const;
    const Client& client();

    AgentController m_controller;
    SecureStorage m_secrets;
    std::unique_ptr<Client> m_client;
    QString m_provider;
    QString m_model;
    QString m_reasoning;
    QString m_apiKey;
    QString m_lastAppId;
    QString m_clientProvider;
    QString m_clientApiKey;
    QString m_error;
};
