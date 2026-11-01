#include "runtime/chat/builderagent.h"

#include "runtime/app/app.h"
#include "runtime/chat/systemprompt.h"

BuilderAgent::BuilderAgent(AppRegistry& registry, QQmlEngine& engine, const QString& model,
                           const QString& notesPath, const ModelConfig& config)
    : m_agent(model, SystemPrompt::text())
    , m_tools(m_agent, registry, notesPath)
    , m_config(config)
{
    m_tools.setEngine(&engine);
}

ModelConfig BuilderAgent::desktopConfig()
{
    return ModelConfig { .model = QStringLiteral("sonnet"),
                         .reasoningEffort = QStringLiteral("high") };
}

QString BuilderAgent::key() { return QStringLiteral("builder"); }

chat::AppAgent BuilderAgent::entry() const
{
    return chat::AppAgent {
        .key = key(),
        .title = QStringLiteral("App builder"),
        .description = QStringLiteral(
            "Creates, changes, repairs and deletes Qanvas apps: writes their QML, checks them on "
            "screen and fixes what fails. Every request about an app goes here. When an app is "
            "open, name its id in the task."),
        .agent = m_agent,
        .followUp = [this] { return followUp(); },
        .config = m_config,
    };
}

QString BuilderAgent::followUp() const { return m_tools.runtimeCheck(App(m_tools.currentApp())); }
