#pragma once

#include <QString>

#include "agent/agent.h"
#include "chat/application/agentcatalog.h"
#include "runtime/chat/apptools.h"

class AppRegistry;
class QQmlEngine;

class BuilderAgent
{
public:
    BuilderAgent(AppRegistry& registry, QQmlEngine& engine, const QString& model,
                 const ModelConfig& config = {});

    static ModelConfig desktopConfig();

    BuilderAgent(const BuilderAgent&) = delete;
    BuilderAgent& operator=(const BuilderAgent&) = delete;

    AppTools& tools() { return m_tools; }
    chat::AppAgent entry() const;
    QString followUp() const;

    static QString key();

private:
    ::Agent m_agent;
    AppTools m_tools;
    ModelConfig m_config;
};
