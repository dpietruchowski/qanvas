#include "runtime/chat/providersettings.h"

#include "chat/ui/chat.h"

#include <QCoreApplication>
#include <QHash>
#include <QSettings>
#include <QVariantMap>
#ifdef QANVAS_KEY_IMPORT
#include <QFile>
#include <QStandardPaths>
#endif

namespace
{
const char* kOpenAiProvider = "OpenAI";
const char* kClaudeProvider = "Claude";
const char* kOpenRouterProvider = "OpenRouter";
const char* kDefaultReasoning = "Minimal";
const char* kSettingsGroup = "agent";
#ifdef QANVAS_KEY_IMPORT
const char* kImportFileName = "/import_keys.ini";
#endif

bool isClaude(const QString& provider) { return provider == QLatin1String(kClaudeProvider); }

QString defaultModel(const QString& provider)
{
    if (isClaude(provider))
        return QStringLiteral("claude-sonnet-5-5");
    if (provider == QLatin1String(kOpenAiProvider))
        return QStringLiteral("gpt-5-mini");
    return QStringLiteral("openai/gpt-5-mini");
}

QString defaultAgentModel(const QString& provider, const QString& agentKey)
{
    static const QHash<QString, QHash<QString, QString>> defaults {
        { QString::fromLatin1(kClaudeProvider),
          { { QStringLiteral("builder"), QStringLiteral("claude-haiku-5-5") } } },
    };
    return defaults.value(provider).value(agentKey);
}

QString providerKey(const QString& provider, const QString& key)
{
    return provider + QLatin1Char('/') + key;
}

QString agentModelSetting(const QString& agentKey)
{
    return QStringLiteral("agentModels/") + agentKey;
}

QString providerUrl(const QString& provider)
{
    if (provider == QLatin1String(kOpenAiProvider))
        return QStringLiteral("https://api.openai.com/v1/chat/completions");
    if (isClaude(provider))
        return QStringLiteral("https://api.anthropic.com/v1/chat/completions");
    return QStringLiteral("https://openrouter.ai/api/v1/chat/completions");
}

bool providerNestsReasoning(const QString& provider)
{
    return provider != QLatin1String(kOpenAiProvider) && !isClaude(provider);
}

QString environmentVariableName(const QString& provider)
{
    if (isClaude(provider))
        return QStringLiteral("CLAUDE_API_KEY");
    return provider == QLatin1String(kOpenAiProvider) ? QStringLiteral("OPENAI_API_KEY")
                                                      : QStringLiteral("OPENROUTER_API_KEY");
}

QString environmentApiKey(const QString& provider)
{
    return QString::fromLocal8Bit(
        qgetenv(environmentVariableName(provider).toLatin1().constData()));
}
}

ProviderSettings::ProviderSettings(const QString& service, QObject* parent)
    : QObject(parent)
    , m_secrets(QCoreApplication::organizationName(), service)
{
    load();
}

QStringList ProviderSettings::providers() const
{
    return { QString::fromLatin1(kOpenAiProvider), QString::fromLatin1(kOpenRouterProvider),
             QString::fromLatin1(kClaudeProvider) };
}

void ProviderSettings::setProvider(const QString& provider)
{
    if (m_provider == provider)
        return;

    m_provider = provider;
    store(QStringLiteral("provider"), provider);
    emit providerChanged();
    emit apiKeyHintChanged();

    applyToChat();
    assignApiKey(m_secrets.retrieve(provider));
}

void ProviderSettings::setApiKey(const QString& apiKey)
{
    if (m_apiKey == apiKey)
        return;

    if (apiKey.isEmpty())
        m_secrets.remove(m_provider);
    else
        m_secrets.store(m_provider, apiKey);

    assignApiKey(apiKey);
}

void ProviderSettings::assignApiKey(const QString& apiKey)
{
    if (m_apiKey == apiKey)
        return;

    m_apiKey = apiKey;
    emit apiKeyChanged();
    emit apiKeyHintChanged();
}

void ProviderSettings::bind(chat::Chat& chat)
{
    m_chat = &chat;
    connect(&chat, &chat::Chat::modelChanged, this,
            [this] { store(providerKey(m_provider, QStringLiteral("model")), m_chat->model()); });
    connect(&chat, &chat::Chat::reasoningEffortChanged, this,
            [this] {
                store(providerKey(m_provider, QStringLiteral("reasoning")),
                      m_chat->reasoningEffort());
            });
    connect(&chat, &chat::Chat::agentsChanged, this, [this] { storeAgentModels(); });
    applyToChat();
}

void ProviderSettings::storeAgentModels()
{
    if (m_applying)
        return;

    for (const QVariant& agent : m_chat->agents())
    {
        const QVariantMap entry = agent.toMap();
        const QString key = entry.value(QStringLiteral("key")).toString();
        store(providerKey(m_provider, agentModelSetting(key)),
              entry.value(QStringLiteral("chosen")).toString());
    }
}

void ProviderSettings::applyToChat()
{
    if (!m_chat)
        return;

    m_applying = true;
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    m_chat->setModel(
        settings.value(providerKey(m_provider, QStringLiteral("model")), defaultModel(m_provider))
            .toString());
    m_chat->setReasoningEffort(settings
                                   .value(providerKey(m_provider, QStringLiteral("reasoning")),
                                          QString::fromLatin1(kDefaultReasoning))
                                   .toString());
    for (const QVariant& agent : m_chat->agents())
    {
        const QString key = agent.toMap().value(QStringLiteral("key")).toString();
        m_chat->setAgentModel(key,
                              settings
                                  .value(providerKey(m_provider, agentModelSetting(key)),
                                         defaultAgentModel(m_provider, key))
                                  .toString());
    }
    m_applying = false;
}

std::unique_ptr<Client> ProviderSettings::createClient() const
{
    const QString key = effectiveApiKey();
    if (key.isEmpty())
        return nullptr;

    auto client = std::make_unique<Client>(providerUrl(m_provider), key);
    client->setReasoningNested(providerNestsReasoning(m_provider));
    return client;
}

chat::ChatModule::Clients ProviderSettings::clients()
{
    return chat::ChatModule::Clients {
        .factory = [this](const ClaudeSession&) { return createClient(); },
        .available = [] { return true; },
        .forget = [](const QString&) {},
        .loginCommand = QString(),
    };
}

#ifdef QANVAS_KEY_IMPORT
void ProviderSettings::importKeys()
{
    const QString path
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + kImportFileName;
    if (!QFile::exists(path))
        return;

    QSettings keys(path, QSettings::IniFormat);
    for (const QString& provider : keys.allKeys())
    {
        const QString key = keys.value(provider).toString();
        if (!key.isEmpty())
            m_secrets.store(provider, key);
    }

    QFile::remove(path);
}
#endif

void ProviderSettings::load()
{
#ifdef QANVAS_KEY_IMPORT
    importKeys();
#endif

    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    setProvider(settings.value(QStringLiteral("provider"), QString::fromLatin1(kOpenAiProvider))
                    .toString());
}

QString ProviderSettings::apiKeyPreview() const
{
    constexpr int head = 8;
    constexpr int tail = 4;

    if (m_apiKey.isEmpty())
        return QString();
    if (m_apiKey.size() < head + tail)
        return QStringLiteral("…");
    return m_apiKey.left(head) + QStringLiteral("…") + m_apiKey.right(tail);
}

QString ProviderSettings::apiKeyHint() const
{
    if (!m_apiKey.isEmpty())
        return QString();
    if (!environmentApiKey(m_provider).isEmpty())
        return environmentVariableName(m_provider);
    if (!m_secrets.isAvailable())
        return QStringLiteral("No keyring — set ") + environmentVariableName(m_provider);
    return QStringLiteral("Paste your key");
}

QString ProviderSettings::effectiveApiKey() const
{
    return m_apiKey.isEmpty() ? environmentApiKey(m_provider) : m_apiKey;
}

void ProviderSettings::store(const QString& key, const QString& value)
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    settings.setValue(key, value);
}
