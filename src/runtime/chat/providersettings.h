#pragma once

#include <memory>

#include <QObject>
#include <QString>
#include <QStringList>

#include "agent/claudesession.h"
#include "agent/client.h"
#include "chat/chatmodule.h"
#include "platform/securestorage.h"

namespace chat
{
class Chat;
}

class ProviderSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList providers READ providers CONSTANT)
    Q_PROPERTY(QString provider READ provider WRITE setProvider NOTIFY providerChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString apiKeyPreview READ apiKeyPreview NOTIFY apiKeyChanged)
    Q_PROPERTY(QString apiKeyHint READ apiKeyHint NOTIFY apiKeyHintChanged)

public:
    explicit ProviderSettings(const QString& service = QStringLiteral("agent"),
                              QObject* parent = nullptr);

    QStringList providers() const;
    QString provider() const { return m_provider; }
    QString apiKey() const { return m_apiKey; }
    QString apiKeyPreview() const;
    QString apiKeyHint() const;

    void setProvider(const QString& provider);
    void setApiKey(const QString& apiKey);

    void bind(chat::Chat& chat);
    std::unique_ptr<Client> createClient() const;
    chat::ChatModule::Clients clients();

signals:
    void providerChanged();
    void apiKeyChanged();
    void apiKeyHintChanged();

private:
    void load();
    void applyToChat();
    void storeAgentModels();
    void assignApiKey(const QString& apiKey);
    void store(const QString& key, const QString& value);
    QString effectiveApiKey() const;
#ifdef QANVAS_KEY_IMPORT
    void importKeys();
#endif

    SecureStorage m_secrets;
    chat::Chat* m_chat = nullptr;
    QString m_provider;
    QString m_apiKey;
    bool m_applying = false;
};
