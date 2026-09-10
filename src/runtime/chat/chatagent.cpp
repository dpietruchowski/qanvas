#include "runtime/chat/chatagent.h"

#include "runtime/app/app.h"
#include "runtime/app/appregistry.h"
#include "runtime/chat/systemprompt.h"
#ifdef QANVAS_CLAUDE_CLI
#include "agent/claudecliclient.h"
#endif

#include <QCoreApplication>
#include <QHash>
#include <QQmlEngine>
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
#ifdef QANVAS_CLAUDE_CLI
const char* kClaudeCliProvider = "Claude CLI";
#endif
const char* kDefaultProvider = "OpenAI";
const char* kDefaultModel = "gpt-5-mini";
const char* kDefaultReasoning = "Minimal";
const int kMaxNudges = 2;
#ifdef QANVAS_KEY_IMPORT
const char* kImportFileName = "/import_keys.ini";
#endif

bool isClaudeCli(const QString& provider)
{
#ifdef QANVAS_CLAUDE_CLI
    return provider == QLatin1String(kClaudeCliProvider);
#else
    Q_UNUSED(provider);
    return false;
#endif
}

bool isClaude(const QString& provider) { return provider == QLatin1String(kClaudeProvider); }

QString defaultModel(const QString& provider)
{
    if (isClaudeCli(provider))
        return QStringLiteral("sonnet");
    if (isClaude(provider))
        return QStringLiteral("claude-sonnet-4-5");
    if (provider == QLatin1String(kOpenAiProvider))
        return QString::fromLatin1(kDefaultModel);
    return QStringLiteral("openai/gpt-5-mini");
}

QString providerKey(const QString& provider, const QString& key)
{
    return provider + QLatin1Char('/') + key;
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

QString errorText(const QString& error)
{
    static const QHash<QString, QString> texts {
        { QStringLiteral("invalid_key"), QStringLiteral("The API key was rejected.") },
        { QStringLiteral("insufficient_credits"),
          QStringLiteral("The account is out of credits.") },
        { QStringLiteral("rate_limit"), QStringLiteral("Rate limited — try again in a moment.") },
        { QStringLiteral("server_error"), QStringLiteral("The provider failed to answer.") },
        { QStringLiteral("network_error"), QStringLiteral("No connection to the provider.") },
    };
    return texts.value(error, QStringLiteral("The request failed."));
}
}

ChatAgent::ChatAgent(QObject* parent)
    : QObject(parent)
    , m_controller(QString::fromLatin1(kDefaultModel), SystemPrompt::text())
    , m_tools(m_controller, AppRegistry::instance(), this)
    , m_thread(m_controller, m_store, this)
    , m_secrets(QCoreApplication::organizationName(), QStringLiteral("agent"))
{
    connect(&m_controller, &AgentController::messagesChanged, this, &ChatAgent::messagesChanged);
    connect(&m_controller, &AgentController::busyChanged, this, &ChatAgent::busyChanged);
    connect(&m_controller, &AgentController::errorOccurred, this,
            [this](const QString& error)
            {
                m_error = errorText(error);
                emit messagesChanged();
            });
    connect(&m_controller, &AgentController::requestFinished, this, &ChatAgent::finishTurn);
    connect(&m_tools, &AppTools::activitiesChanged, this, &ChatAgent::activitiesChanged);
    connect(&m_tools, &AppTools::appShown, this,
            [this](const QString& id)
            {
                m_thread.bind(id);
                emit appShown(id);
            });
    connect(&m_thread, &ChatThread::appChanged, this,
            [this]
            {
                store(QStringLiteral("lastApp"), m_thread.app());
                emit lastAppIdChanged();
            });
    connect(&AppRegistry::instance(), &AppRegistry::appRemoving, this,
            [this](const QString& id) { m_thread.forget(id); });

    load();
}

void ChatAgent::componentComplete() { m_tools.setEngine(qmlEngine(this)); }

void ChatAgent::openApp(const QString& id)
{
    if (m_controller.isBusy() || id == m_thread.app())
        return;

    m_error.clear();
    m_tools.clearActivities();
    m_thread.open(id);
}

QStringList ChatAgent::providers() const
{
    QStringList providers { QString::fromLatin1(kOpenAiProvider), QStringLiteral("OpenRouter"),
                            QString::fromLatin1(kClaudeProvider) };
#ifdef QANVAS_CLAUDE_CLI
    if (ClaudeCliClient::isAvailable())
        providers.append(QString::fromLatin1(kClaudeCliProvider));
#endif
    return providers;
}

QVariantList ChatAgent::messages() const
{
    QVariantList messages = m_controller.messages();
    if (!m_error.isEmpty())
    {
        messages.append(QVariantMap { { QStringLiteral("sender"), QStringLiteral("assistant") },
                                      { QStringLiteral("text"), m_error } });
    }
    return messages;
}

void ChatAgent::setProvider(const QString& provider)
{
    if (m_provider == provider)
        return;

    m_provider = provider;
    store(QStringLiteral("provider"), provider);
    emit providerChanged();
    emit apiKeyHintChanged();

    loadProviderSettings();
    setApiKey(m_secrets.retrieve(provider));
}

void ChatAgent::loadProviderSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("agent"));
    setModel(
        settings.value(providerKey(m_provider, QStringLiteral("model")), defaultModel(m_provider))
            .toString());
    setReasoning(settings
                     .value(providerKey(m_provider, QStringLiteral("reasoning")),
                            QString::fromLatin1(kDefaultReasoning))
                     .toString());
}

void ChatAgent::setModel(const QString& model)
{
    if (m_model == model)
        return;

    m_model = model;
    m_controller.setModel(model);
    store(providerKey(m_provider, QStringLiteral("model")), model);
    emit modelChanged();
}

void ChatAgent::setReasoning(const QString& reasoning)
{
    if (m_reasoning == reasoning)
        return;

    m_reasoning = reasoning;
    m_controller.setReasoningEffort(reasoning.toLower());
    store(providerKey(m_provider, QStringLiteral("reasoning")), reasoning);
    emit reasoningChanged();
}

void ChatAgent::setApiKey(const QString& apiKey)
{
    if (m_apiKey == apiKey)
        return;

    m_apiKey = apiKey;
    if (apiKey.isEmpty())
    {
        m_secrets.remove(m_provider);
    }
    else
    {
        m_secrets.store(m_provider, apiKey);
    }
    emit apiKeyChanged();
    emit apiKeyHintChanged();
}

void ChatAgent::send(const QString& text)
{
    if (m_controller.isBusy())
        return;

    m_error.clear();
    m_tools.clearActivities();

    if (!isClaudeCli(m_provider) && effectiveApiKey().isEmpty())
    {
        m_error = QStringLiteral("Add an API key in settings first.");
        emit messagesChanged();
        return;
    }

    m_nudges = 0;
    m_thread.remember(text);
    m_controller.request(client(), text);
}

void ChatAgent::finishTurn()
{
    const App app(m_thread.app());
    if (m_nudges >= kMaxNudges || m_tools.activities().isEmpty() || !app.exists())
        return;

    const QString pending = m_tools.unfinished(app, true);
    if (pending.isEmpty())
        return;

    ++m_nudges;
    const QString text = QStringLiteral("Runtime check — the app is not finished:\n%1\nGo on "
                                        "until read_log lists nothing undone, then one sentence.")
                             .arg(pending);
    m_thread.remember(text);
    m_controller.request(client(), text);
}

void ChatAgent::clear()
{
    if (m_controller.isBusy())
        return;

    m_error.clear();
    m_tools.clearActivities();
    m_thread.reset();
}

#ifdef QANVAS_KEY_IMPORT
void ChatAgent::importKeys()
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

void ChatAgent::load()
{
#ifdef QANVAS_KEY_IMPORT
    importKeys();
#endif

    QSettings settings;
    settings.beginGroup(QStringLiteral("agent"));
    setProvider(settings.value(QStringLiteral("provider"), QString::fromLatin1(kDefaultProvider))
                    .toString());

    const QString lastApp = settings.value(QStringLiteral("lastApp")).toString();
    m_thread.open(App(lastApp).exists() ? lastApp : QString());
}

QString ChatAgent::apiKeyPreview() const
{
    constexpr int head = 8;
    constexpr int tail = 4;

    if (m_apiKey.isEmpty())
        return QString();
    if (m_apiKey.size() < head + tail)
        return QStringLiteral("…");
    return m_apiKey.left(head) + QStringLiteral("…") + m_apiKey.right(tail);
}

QString ChatAgent::apiKeyHint() const
{
    if (isClaudeCli(m_provider))
        return QStringLiteral("Not used by the CLI");
    if (!m_apiKey.isEmpty())
        return QString();
    if (!environmentApiKey(m_provider).isEmpty())
        return environmentVariableName(m_provider);
    if (!m_secrets.isAvailable())
        return QStringLiteral("No keyring — set ") + environmentVariableName(m_provider);
    return QStringLiteral("Paste your key");
}

QString ChatAgent::effectiveApiKey() const
{
    return m_apiKey.isEmpty() ? environmentApiKey(m_provider) : m_apiKey;
}

void ChatAgent::store(const QString& key, const QString& value)
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("agent"));
    settings.setValue(key, value);
}

const Client& ChatAgent::client()
{
    const QString key = effectiveApiKey();
    if (!m_client || m_clientProvider != m_provider || m_clientApiKey != key)
    {
#ifdef QANVAS_CLAUDE_CLI
        if (isClaudeCli(m_provider))
        {
            m_client = std::make_unique<ClaudeCliClient>();
            m_clientProvider = m_provider;
            m_clientApiKey = key;
            return *m_client;
        }
#endif
        m_client = std::make_unique<Client>(providerUrl(m_provider), key);
        m_client->setReasoningNested(providerNestsReasoning(m_provider));
        m_clientProvider = m_provider;
        m_clientApiKey = key;
    }
    return *m_client;
}
