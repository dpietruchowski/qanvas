#include "chatagent.h"

#include "appregistry.h"
#ifdef QANVAS_CLAUDE_CLI
#include "agent/claudecliclient.h"
#endif

#include <QCoreApplication>
#include <QHash>
#include <QQmlEngine>
#include <QSettings>
#include <QVariantMap>

namespace
{
const char* kSystemPrompt
    = "You are the assistant of Qanvas, a runtime that builds small QML apps on request. "
      "Keep answers short and concrete, and ask for the one detail you are missing instead of "
      "guessing.\n"
      "When the user wants an app, write it and call create_app; the runtime loads it at once. "
      "Give it a short title in the language the user writes in — it names the app in the "
      "runtime's app list.\n"
      "An app is a directory of QML files. Main.qml is the entry point; any other Name.qml beside "
      "it is a component used by its name, with no import. Split a screen into components once "
      "Main.qml outgrows a screenful.\n"
      "To change an app that already exists, go through its files: search_files to find the place, "
      "read_file to see it, edit_file to change a fragment, write_file for a new or fully "
      "rewritten "
      "file, delete_file to drop one. Prefer an anchored edit_file over rewriting a whole file, "
      "and "
      "never rewrite a file you have not read.\n"
      "Every file has exactly three imports: QtQuick, QtQuick.Layouts and "
      "Qanvas.Components. Main.qml's root object is a QanvasPage. Every visible thing on the "
      "screen "
      "is one "
      "of the Qanvas components listed below — no Rectangle, Text, Item, Image, ListView, no "
      "QtQuick.Controls, no import of anything else. QtQuick is there for the invisible parts "
      "only: "
      "Timer, Repeater, QtObject, property, function, Component.onCompleted. The components carry "
      "the runtime's look, so never set colours, fonts, sizes, radii, paddings or spacing on them; "
      "pick a variant by writing words in type instead. Children of QanvasPage and of the layout "
      "components stack in a column; place them with Layout.fillWidth, Layout.fillHeight and "
      "Layout.preferredHeight. If the catalogue has no component for what you want, build it out "
      "of "
      "the ones it does have — a card with a row inside covers most of it. There is no C++, no "
      "network and no filesystem.\n"
      "QanvasPage{title scrollable showBack onBack} QanvasPanel{title open() close()} "
      "QanvasCard{clickable onClicked} QanvasSection{title}\n"
      "QanvasColumn QanvasRow QanvasGrid{columns} QanvasFlow QanvasScroll QanvasSpacer "
      "QanvasSeparator\n"
      "QanvasText{text type:title|subtitle|body|caption|muted} "
      "QanvasIcon{iconName type:small|medium|large}\n"
      "QanvasButton{text iconName onClicked type} — type takes one colour word "
      "(primary|secondary|danger|success|ghost) and optionally a shape word "
      "(small|large|circle|rounded), as in type: \"danger small\". QanvasFab{iconName onClicked}\n"
      "QanvasInput{text placeholder clearOnSubmit onSubmitted(text) type:text|number|search} "
      "QanvasTextArea{text readOnly}\n"
      "QanvasCheckBox{text checked} QanvasSwitch{text checked} QanvasSlider{value from to} "
      "QanvasSelect{options value onSelected(value)}\n"
      "QanvasList{model textRole subtitleRole iconRole removable onItemClicked(index,item) "
      "onRemoveClicked(index,item)} QanvasListItem{title subtitle iconName removable checkable "
      "checked onClicked onRemoveClicked onToggled(checked)}\n"
      "QanvasGridView{model columns textRole onItemClicked(index,item)} "
      "QanvasEmpty{text iconName}\n"
      "QanvasDialog{title message acceptText rejectText showCancel type:danger|success open() "
      "onAccepted} QanvasToast{show(text)} QanvasBusy{running}\n"
      "iconName picks from: plus minus remove check close edit copy search settings list menu "
      "star star-full timer calendar home user info warning error success chat send globe next "
      "previous chevron-down chevron-up chevron-right.\n"
      "Data lives in the app's own SQLite database, reached through the App object: "
      "App.exec(sql, params) runs CREATE TABLE / INSERT / UPDATE / DELETE and App.query(sql, "
      "params) returns a list of maps, one per row. params is a list for ? placeholders. "
      "Create the tables the app needs with CREATE TABLE IF NOT EXISTS in Component.onCompleted. "
      "App.revision changes after every exec, so bind models as "
      "model: (App.revision, App.query(\"SELECT ...\")) to keep them fresh.\n"
      "When a tool answers that the app does not load or raised errors, read the file and line "
      "it names and fix it before you answer; never leave the user with a broken app. When the "
      "user says something in the app misbehaves, read_log shows the errors it raised. "
      "delete_app removes an app for good, so call it only when the user asks to remove one.\n"
      "After the call, say in one sentence what the app does.";

const char* kOpenAiProvider = "OpenAI";
const char* kClaudeCliProvider = "Claude CLI";
const char* kDefaultProvider = "OpenAI";
const char* kDefaultModel = "gpt-5-mini";
const char* kDefaultReasoning = "Minimal";

bool isClaudeCli(const QString& provider)
{
#ifdef QANVAS_CLAUDE_CLI
    return provider == QLatin1String(kClaudeCliProvider);
#else
    Q_UNUSED(provider);
    return false;
#endif
}

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
    , m_tools(m_controller, AppRegistry::instance(), this)
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
    connect(&m_tools, &AppTools::activitiesChanged, this, &ChatAgent::activitiesChanged);
    connect(&m_tools, &AppTools::appShown, this, &ChatAgent::showApp);
    connect(&AppRegistry::instance(), &AppRegistry::appRemoving, this,
            [this](const QString& id)
            {
                if (id != m_lastAppId)
                    return;

                m_lastAppId.clear();
                emit lastAppIdChanged();
            });

    load();
}

void ChatAgent::componentComplete() { m_tools.setEngine(qmlEngine(this)); }

void ChatAgent::showApp(const QString& id)
{
    m_lastAppId = id;
    emit lastAppIdChanged();
    emit appShown(id);
}

QStringList ChatAgent::providers() const
{
    QStringList providers { QString::fromLatin1(kOpenAiProvider), QStringLiteral("OpenRouter") };
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
    m_tools.clearActivities();

    if (!isClaudeCli(m_provider) && effectiveApiKey().isEmpty())
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
    m_tools.clearActivities();
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
