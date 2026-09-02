#include "chatagent.h"

#include <QCoreApplication>
#include <QHash>
#include <QSettings>
#include <QVariantMap>

namespace
{
const char* kSystemPrompt = "You are the assistant of Qanvas, a runtime that builds small QML apps "
                            "on request. Keep answers short and concrete, and ask for the one "
                            "detail you are missing instead of guessing.";

const char* kDefaultProvider = "OpenAI";
const char* kDefaultModel = "gpt-5-mini";
const char* kDefaultReasoning = "Minimal";

QString providerUrl(const QString& provider)
{
    if (provider == QStringLiteral("OpenAI"))
        return QStringLiteral("https://api.openai.com/v1/chat/completions");
    return QStringLiteral("https://openrouter.ai/api/v1/chat/completions");
}

bool providerNestsReasoning(const QString& provider)
{
    return provider != QStringLiteral("OpenAI");
}

QString environmentVariableName(const QString& provider)
{
    return provider == QStringLiteral("OpenAI") ? QStringLiteral("OPENAI_API_KEY")
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
    , m_controller(QString::fromLatin1(kDefaultModel), QString::fromLatin1(kSystemPrompt))
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

    load();
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

    setApiKey(m_secrets.retrieve(provider));
}

void ChatAgent::setModel(const QString& model)
{
    if (m_model == model)
        return;

    m_model = model;
    m_controller.setModel(model);
    store(QStringLiteral("model"), model);
    emit modelChanged();
}

void ChatAgent::setReasoning(const QString& reasoning)
{
    if (m_reasoning == reasoning)
        return;

    m_reasoning = reasoning;
    m_controller.setReasoningEffort(reasoning.toLower());
    store(QStringLiteral("reasoning"), reasoning);
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

    if (effectiveApiKey().isEmpty())
    {
        m_error = QStringLiteral("Add an API key in settings first.");
        emit messagesChanged();
        return;
    }

    m_controller.request(client(), text);
}

void ChatAgent::clear()
{
    m_error.clear();
    m_controller.clear(QString());
}

void ChatAgent::load()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("agent"));
    setProvider(settings.value(QStringLiteral("provider"), QString::fromLatin1(kDefaultProvider))
                    .toString());
    setModel(
        settings.value(QStringLiteral("model"), QString::fromLatin1(kDefaultModel)).toString());
    setReasoning(settings.value(QStringLiteral("reasoning"), QString::fromLatin1(kDefaultReasoning))
                     .toString());
}

QString ChatAgent::apiKeyHint() const
{
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
        m_client = std::make_unique<Client>(providerUrl(m_provider), key);
        m_client->setReasoningNested(providerNestsReasoning(m_provider));
        m_clientProvider = m_provider;
        m_clientApiKey = key;
    }
    return *m_client;
}
