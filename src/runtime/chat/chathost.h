#pragma once

#include <memory>

#include <QObject>
#include <QSet>
#include <QString>
#include <QUuid>

#include "chat/chatmodule.h"
#include "runtime/chat/builderagent.h"
#include "runtime/chat/providersettings.h"
#include "runtime/storage/appconversations.h"

class AppRegistry;
class BackendWorker;
class QmlRegistrator;
class QQmlEngine;

class ChatHost : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QObject* providerSettings READ providerSettings CONSTANT)
    Q_PROPERTY(QString lastAppId READ lastAppId WRITE setLastAppId NOTIFY lastAppIdChanged)

public:
    struct Config
    {
        QString dataDirectory;
        QString settingsOrganization;
        QString settingsApplication;
    };

    ChatHost(const Config& config, QQmlEngine& engine, BackendWorker& worker, AppRegistry& registry,
             QObject* parent = nullptr);
    ChatHost(const Config& config, chat::ChatModule::Clients clients, QQmlEngine& engine,
             BackendWorker& worker, AppRegistry& registry, QObject* parent = nullptr);
    ~ChatHost() override;

    bool isReady() const { return m_module != nullptr; }
    bool busy() const { return !m_running.isEmpty(); }
    QObject* providerSettings() const { return m_providers.get(); }
    chat::Chat* chat() const;
    QString lastAppId() const;
    void setLastAppId(const QString& id);

    Q_INVOKABLE QString conversationFor(const QString& appId) const;
    Q_INVOKABLE QString appFor(const QString& conversationId) const;
    Q_INVOKABLE void bind(const QString& appId, const QString& conversationId);

    void registerQml(QmlRegistrator& registrator);

signals:
    void busyChanged();
    void lastAppIdChanged();
    void appShown(const QString& id);

private:
    void start(const Config& config, chat::ChatModule::Clients clients, QQmlEngine& engine,
               BackendWorker& worker);
    void turnStarted(const QUuid& conversationId);
    void turnFinished(const QUuid& conversationId);

    QString readLastApp() const;

    Config m_config;
    AppRegistry& m_registry;
    QString m_lastApp;
    AppConversations m_conversations;
    std::unique_ptr<ProviderSettings> m_providers;
    std::unique_ptr<BuilderAgent> m_builder;
    std::unique_ptr<chat::ChatModule> m_module;
    QSet<QUuid> m_running;
};
