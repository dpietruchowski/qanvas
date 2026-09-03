#pragma once

#include <QObject>
#include <QString>

#include "agent/agentcontroller.h"

class ChatStore;

class ChatThread : public QObject
{
    Q_OBJECT

public:
    ChatThread(AgentController& controller, ChatStore& store, QObject* parent = nullptr);

    QString app() const { return m_app; }

    void open(const QString& app);
    void bind(const QString& app);
    void remember(const QString& text);
    void forget(const QString& app);
    void reset();

signals:
    void appChanged();

private:
    void setApp(const QString& app);
    void rememberReply();

    AgentController& m_controller;
    ChatStore& m_store;
    QString m_app;
};
