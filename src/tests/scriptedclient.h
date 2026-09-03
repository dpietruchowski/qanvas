#pragma once

#include <vector>

#include <QEventLoop>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "agent/agentcontroller.h"
#include "agent/client.h"
#include "agent/structures.h"

inline void requestAndWait(AgentController& controller, const Client& client, const QString& text)
{
    QEventLoop loop;
    QObject::connect(&controller, &AgentController::busyChanged, &loop,
                     [&controller, &loop]
                     {
                         if (!controller.isBusy())
                             loop.quit();
                     });
    controller.request(client, text);
    if (controller.isBusy())
        loop.exec();
}

class ScriptedClient : public Client
{
public:
    ScriptedClient()
        : Client(QStringLiteral("http://scripted.invalid"), QStringLiteral("test-key"))
    {
    }

    static Completion reply(const QString& text)
    {
        Choice choice;
        choice.index = 0;
        choice.finish_reason = QStringLiteral("stop");
        choice.message = Message { .role = QStringLiteral("assistant"), .content = text };

        Completion completion;
        completion.choices.append(choice);
        return completion;
    }

    static Completion toolCall(const QString& name, const QVariantMap& arguments)
    {
        ToolCall call;
        call.id = QStringLiteral("call_%1").arg(name);
        call.name = name;
        call.type = QStringLiteral("function");
        call.arguments = arguments;

        Choice choice;
        choice.index = 0;
        choice.finish_reason = QStringLiteral("tool_calls");
        choice.message = Message { .role = QStringLiteral("assistant"), .content = QString() };
        choice.message.tool_calls.append(call);

        Completion completion;
        completion.choices.append(choice);
        return completion;
    }

    QString lastToolResult() const
    {
        for (auto it = lastMessages.crbegin(); it != lastMessages.crend(); ++it)
        {
            if (it->role == QStringLiteral("tool"))
                return it->content;
        }
        return QString();
    }

    Completion createCompletion(const ModelConfig&, const Messages& messages,
                                const ToolsMap&) const override
    {
        lastMessages = messages;
        return script.at(calls++);
    }

    void createCompletionAsync(const ModelConfig&, const Messages& messages, const ToolsMap&,
                               const CompletionCreatedCallback& callback) const override
    {
        lastMessages = messages;
        Completion completion = script.at(calls++);
        callback(completion);
    }

    std::vector<Completion> script;
    mutable int calls = 0;
    mutable Messages lastMessages;
};
