#include "runtime/chat/chathost.h"

#include "qmlutils/qmlregistrator.h"
#include "runtime/app/app.h"
#include "runtime/app/appregistry.h"

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QUuid>

namespace
{
const QString kAppName = QStringLiteral("Qanvas");
const QString kAppPurpose = QStringLiteral(
    "Qanvas is a runtime that runs small QML apps — timers, trackers, quizzes, lists, notes — "
    "which are written on request and appear on the user's screen as soon as they are built. "
    "Every request to create, change, repair, inspect or delete an app goes to the builder "
    "agent, which has the tools; you cannot touch apps yourself. Write each task in the "
    "language the user writes in. Answer questions about Qanvas and plain conversation "
    "yourself, briefly.");
const QString kChatDatabase = QStringLiteral("chat.db");
const QString kBuilderNotes = QStringLiteral("builder-notes.md");
const QString kClaudeDirectory = QStringLiteral("claude");
const QString kSettingsGroup = QStringLiteral("chat");
const QString kLastAppKey = QStringLiteral("lastApp");

chat::ChatModule::Config moduleConfig(const ChatHost::Config& config, BackendWorker& worker)
{
    return chat::ChatModule::Config {
        .databasePath = QDir(config.dataDirectory).filePath(kChatDatabase),
        .workingDirectory = QDir(config.dataDirectory).filePath(kClaudeDirectory),
        .settingsOrganization = config.settingsOrganization,
        .settingsApplication = config.settingsApplication,
        .settingsGroup = kSettingsGroup,
        .worker = &worker,
        .appName = kAppName,
        .appPurpose = kAppPurpose,
    };
}
}

ChatHost::ChatHost(const Config& config, QQmlEngine& engine, BackendWorker& worker,
                   AppRegistry& registry, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_registry(registry)
    , m_lastApp(readLastApp())
{
#ifdef Q_OS_ANDROID
    m_providers = std::make_unique<ProviderSettings>();
    start(config, m_providers->clients(), {}, engine, worker);
#else
    start(config,
          chat::ChatModule::claudeClients(QDir(config.dataDirectory).filePath(kClaudeDirectory)),
          BuilderAgent::desktopConfig(), engine, worker);
#endif
}

ChatHost::ChatHost(const Config& config, chat::ChatModule::Clients clients, QQmlEngine& engine,
                   BackendWorker& worker, AppRegistry& registry, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_registry(registry)
    , m_lastApp(readLastApp())
{
    start(config, std::move(clients), {}, engine, worker);
}

ChatHost::~ChatHost() = default;

void ChatHost::start(const Config& config, chat::ChatModule::Clients clients,
                     const ModelConfig& builderConfig, QQmlEngine& engine, BackendWorker& worker)
{
    QDir().mkpath(config.dataDirectory);
    m_module = chat::ChatModule::create(moduleConfig(config, worker), std::move(clients));
    if (!m_module)
    {
        qWarning() << "Chat storage could not be opened";
        return;
    }

    m_builder = std::make_unique<BuilderAgent>(m_registry, engine, m_module->chat().model(),
                                               QDir(config.dataDirectory).filePath(kBuilderNotes),
                                               builderConfig);
    m_module->addAgent(m_builder->entry());
    m_builder->tools().setCurrentApp(lastAppId());
    if (m_providers)
        m_providers->bind(m_module->chat());

    connect(&m_module->chat().runner(), &chat::TurnRunner::turnStarted, this,
            &ChatHost::turnStarted);
    connect(&m_module->chat().runner(), &chat::TurnRunner::turnFinished, this,
            &ChatHost::turnFinished);
    connect(&m_module->chat().runner(), &chat::TurnRunner::conversationRemoved, this,
            &ChatHost::conversationRemoved);
    connect(&m_builder->tools(), &AppTools::appShown, this, &ChatHost::showApp);
    connect(&m_registry, &AppRegistry::appRemoving, this,
            [this](const QString& id)
            {
                m_conversations.remove(id);
                if (id == m_lastApp)
                    setLastAppId(QString());
            });
}

QString ChatHost::lastAppId() const { return App(m_lastApp).exists() ? m_lastApp : QString(); }

void ChatHost::setLastAppId(const QString& id)
{
    if (id == m_lastApp)
        return;

    m_lastApp = id;
    QSettings settings(m_config.settingsOrganization, m_config.settingsApplication);
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kLastAppKey, id);
    if (m_builder)
        m_builder->tools().setCurrentApp(lastAppId());
    emit lastAppIdChanged();
}

QString ChatHost::readLastApp() const
{
    QSettings settings(m_config.settingsOrganization, m_config.settingsApplication);
    settings.beginGroup(kSettingsGroup);
    return settings.value(kLastAppKey).toString();
}

chat::Chat* ChatHost::chat() const { return m_module ? &m_module->chat() : nullptr; }

QString ChatHost::conversationFor(const QString& appId) const
{
    return m_conversations.conversationFor(appId);
}

QString ChatHost::appFor(const QString& conversationId) const
{
    return m_conversations.appFor(conversationId);
}

void ChatHost::bind(const QString& appId, const QString& conversationId)
{
    m_conversations.bind(appId, conversationId);
}

void ChatHost::registerQml(QmlRegistrator& registrator)
{
    if (m_module)
        m_module->registerQml(registrator);
    registrator.registerSingletonInstance("ChatHost", this);
}

void ChatHost::showApp(const QString& appId)
{
    m_conversations.bind(appId, m_turnConversation);
    emit appShown(appId, m_turnConversation);
}

void ChatHost::turnStarted(const QUuid& conversationId)
{
    const bool wasBusy = busy();
    m_turnConversation = conversationId.toString(QUuid::WithoutBraces);
    m_running.insert(conversationId);
    m_builder->tools().clearActivities();
    if (!wasBusy)
        emit busyChanged();
}

void ChatHost::turnFinished(const QUuid& conversationId)
{
    if (m_running.remove(conversationId) && !busy())
        emit busyChanged();
}

void ChatHost::conversationRemoved(const QUuid& conversationId)
{
    m_conversations.removeConversation(conversationId.toString(QUuid::WithoutBraces));
}
