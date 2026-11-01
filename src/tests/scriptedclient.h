#pragma once

#include <vector>

#include <QString>
#include <QTest>
#include <QVariantMap>

#include "agent/agent.h"
#include "agent/client.h"
#include "agent/structures.h"

inline QString requestAndWait(::Agent& agent, const Client& client, const QString& text)
{
    constexpr int waitMilliseconds = 5000;

    QString answer;
    agent.requestAsync(client, text,
                       [&answer](const QString& response)
                       {
                           if (!response.isEmpty())
                               answer = response;
                       });
    const bool answered
        = QTest::qWaitFor([&answer] { return !answer.isEmpty(); }, waitMilliseconds);
    return answered ? answer : QString();
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

    QStringList toolResults() const
    {
        QStringList results;
        for (const Message& message : lastMessages)
        {
            if (message.role == QStringLiteral("tool"))
                results.append(message.content);
        }
        return results;
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
                                const ToolsMap& tools) const override
    {
        lastMessages = messages;
        lastTools = tools;
        return script.at(calls++);
    }

    void createCompletionAsync(const ModelConfig&, const Messages& messages, const ToolsMap& tools,
                               const CompletionCreatedCallback& callback) const override
    {
        lastMessages = messages;
        lastTools = tools;
        Completion completion = script.at(calls++);
        callback(completion);
    }

    std::vector<Completion> script;
    mutable int calls = 0;
    mutable Messages lastMessages;
    mutable ToolsMap lastTools;
};
