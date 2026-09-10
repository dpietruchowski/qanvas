#pragma once

#include <memory>

#include <QObject>
#include <QQmlParserStatus>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include "agent/agentcontroller.h"
#include "agent/client.h"
#include "platform/securestorage.h"
#include "runtime/chat/apptools.h"
#include "runtime/chat/chatthread.h"
#include "runtime/storage/chatstore.h"

class ChatAgent : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QStringList providers READ providers CONSTANT)
    Q_PROPERTY(QString provider READ provider WRITE setProvider NOTIFY providerChanged)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString reasoning READ reasoning WRITE setReasoning NOTIFY reasoningChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString apiKeyPreview READ apiKeyPreview NOTIFY apiKeyChanged)
    Q_PROPERTY(QString apiKeyHint READ apiKeyHint NOTIFY apiKeyHintChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QVariantList activities READ activities NOTIFY activitiesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastAppId READ lastAppId NOTIFY lastAppIdChanged)

public:
    explicit ChatAgent(QObject* parent = nullptr);

    QStringList providers() const;
    QString provider() const { return m_provider; }
    QString model() const { return m_model; }
    QString reasoning() const { return m_reasoning; }
    QString apiKey() const { return m_apiKey; }
    QString apiKeyPreview() const;
    QString apiKeyHint() const;
    QVariantList messages() const;
    QVariantList activities() const { return m_tools.activities(); }
    bool busy() const { return m_controller.isBusy(); }
    QString lastAppId() const { return m_thread.app(); }

    void setProvider(const QString& provider);
    void setModel(const QString& model);
    void setReasoning(const QString& reasoning);
    void setApiKey(const QString& apiKey);

    Q_INVOKABLE void send(const QString& text);
    Q_INVOKABLE void openApp(const QString& id);
    Q_INVOKABLE void clear();

    void classBegin() override { }
    void componentComplete() override;

signals:
    void providerChanged();
    void modelChanged();
    void reasoningChanged();
    void apiKeyChanged();
    void apiKeyHintChanged();
    void messagesChanged();
    void activitiesChanged();
    void busyChanged();
    void lastAppIdChanged();
    void appShown(const QString& id);

private:
    void load();
    void loadProviderSettings();
    void finishTurn();
    QString withOpenApp(const QString& text) const;
#ifdef QANVAS_KEY_IMPORT
    void importKeys();
#endif
    void store(const QString& key, const QString& value);
    QString effectiveApiKey() const;
    const Client& client();

    AgentController m_controller;
    AppTools m_tools;
    ChatStore m_store;
    ChatThread m_thread;
    SecureStorage m_secrets;
    std::unique_ptr<Client> m_client;
    QString m_provider;
    QString m_model;
    QString m_reasoning;
    QString m_apiKey;
    QString m_clientProvider;
    QString m_clientApiKey;
    QString m_error;
    int m_nudges = 0;
};
