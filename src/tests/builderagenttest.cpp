#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include <QQmlEngine>
#include <QTemporaryDir>
#include <QVariantMap>

#include "agent/agent.h"
#include "runtime/app/app.h"
#include "runtime/app/appregistry.h"
#include "runtime/chat/builderagent.h"
#include "runtime/chat/systemprompt.h"
#include "scriptedclient.h"

namespace
{
const QString kMainQml = QStringLiteral("import QtQuick\nItem {}\n");

QVariantMap createApp()
{
    return QVariantMap { { QStringLiteral("id"), QStringLiteral("notes") },
                         { QStringLiteral("title"), QStringLiteral("Notes") },
                         { QStringLiteral("qml"), kMainQml } };
}

QVariantMap writeUnusedScreen()
{
    return QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") },
                         { QStringLiteral("path"), QStringLiteral("Card.qml") },
                         { QStringLiteral("content"), kMainQml } };
}
}

class BuilderAgentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit());
        registry = std::make_unique<AppRegistry>(nullptr);
        engine.setOutputWarningsToStandardError(false);
        builder = std::make_unique<BuilderAgent>(*registry, engine, QStringLiteral("test-model"));
    }

    void TearDown() override
    {
        builder.reset();
        registry.reset();
        qunsetenv("QANVAS_APPS_ROOT");
    }

    void run(std::vector<Completion> script)
    {
        client.script = std::move(script);
        client.calls = 0;
        ::Agent agent = builder->entry().agent;
        ASSERT_FALSE(requestAndWait(agent, client, QStringLiteral("go")).isEmpty());
    }

    QTemporaryDir root;
    QQmlEngine engine;
    std::unique_ptr<AppRegistry> registry;
    std::unique_ptr<BuilderAgent> builder;
    ScriptedClient client;
};

TEST_F(BuilderAgentTest, IsRegisteredUnderTheBuilderKeyWithADescriptionOfItsJob)
{
    const chat::AppAgent entry = builder->entry();

    EXPECT_EQ(entry.key, QStringLiteral("builder"));
    EXPECT_EQ(BuilderAgent::key(), entry.key);
    EXPECT_FALSE(entry.title.isEmpty());
    EXPECT_TRUE(entry.description.contains(QStringLiteral("app")));
    EXPECT_TRUE(entry.description.contains(QStringLiteral("id")));
    EXPECT_TRUE(static_cast<bool>(entry.followUp));
}

TEST_F(BuilderAgentTest, WithoutAConfigTheEntryFollowsTheModelAndEffortOfTheChat)
{
    const chat::AppAgent entry = builder->entry();

    EXPECT_TRUE(entry.config.model.isEmpty());
    EXPECT_TRUE(entry.config.reasoningEffort.isEmpty());
}

TEST_F(BuilderAgentTest, TheEntryCarriesTheModelAndEffortOfItsConfig)
{
    BuilderAgent configured(*registry, engine, QStringLiteral("test-model"),
                            ModelConfig { .model = QStringLiteral("big-model"),
                                          .reasoningEffort = QStringLiteral("max") });

    const chat::AppAgent entry = configured.entry();

    EXPECT_EQ(entry.config.model, QStringLiteral("big-model"));
    EXPECT_EQ(entry.config.reasoningEffort, QStringLiteral("max"));
}

TEST_F(BuilderAgentTest, TheDesktopConfigIsOpusOnHighEffort)
{
    const ModelConfig config = BuilderAgent::desktopConfig();

    EXPECT_EQ(config.model, QStringLiteral("opus"));
    EXPECT_EQ(config.reasoningEffort, QStringLiteral("high"));

    BuilderAgent desktop(*registry, engine, QStringLiteral("test-model"), config);
    EXPECT_EQ(desktop.entry().config.model, QStringLiteral("opus"));
    EXPECT_EQ(desktop.entry().config.reasoningEffort, QStringLiteral("high"));
}

TEST_F(BuilderAgentTest, RunsOnTheSystemPromptWithTheAppTools)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), createApp()),
          ScriptedClient::reply(QStringLiteral("done")) });

    ASSERT_FALSE(client.lastMessages.isEmpty());
    EXPECT_EQ(client.lastMessages.first().content, SystemPrompt::text());
    EXPECT_TRUE(App(QStringLiteral("notes")).exists());
    EXPECT_EQ(builder->tools().currentApp(), QStringLiteral("notes"));
}

TEST_F(BuilderAgentTest, FollowUpIsEmptyBeforeAnythingWasDone)
{
    EXPECT_TRUE(builder->followUp().isEmpty());
}

TEST_F(BuilderAgentTest, FollowUpIsEmptyWhenTheAppIsFinished)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), createApp()),
          ScriptedClient::reply(QStringLiteral("done")) });

    EXPECT_TRUE(builder->followUp().isEmpty());
}

TEST_F(BuilderAgentTest, FollowUpNamesWhatTheBuilderLeftUndone)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), createApp()),
          ScriptedClient::toolCall(QStringLiteral("write_file"), writeUnusedScreen()),
          ScriptedClient::reply(QStringLiteral("done")) });

    const QString text = builder->followUp();

    EXPECT_TRUE(text.startsWith(QStringLiteral("Runtime check — the app is not finished:")))
        << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("cannot reach them: Card.qml"))) << qPrintable(text);
    EXPECT_TRUE(text.endsWith(QStringLiteral("Go on until read_log lists nothing undone, then one "
                                             "sentence.")))
        << qPrintable(text);
}

TEST_F(BuilderAgentTest, TheFollowUpOfTheEntryGivesTheSameText)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), createApp()),
          ScriptedClient::toolCall(QStringLiteral("write_file"), writeUnusedScreen()),
          ScriptedClient::reply(QStringLiteral("done")) });

    EXPECT_EQ(builder->entry().followUp(), builder->followUp());
    EXPECT_FALSE(builder->entry().followUp().isEmpty());
}

TEST_F(BuilderAgentTest, FollowUpForgetsTheTurnOnceItsStateIsCleared)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), createApp()),
          ScriptedClient::toolCall(QStringLiteral("write_file"), writeUnusedScreen()),
          ScriptedClient::reply(QStringLiteral("done")) });

    builder->tools().clearActivities();

    EXPECT_TRUE(builder->followUp().isEmpty());
}

TEST_F(BuilderAgentTest, FollowUpIsEmptyForAnAppThatWasDeleted)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), createApp()),
          ScriptedClient::toolCall(QStringLiteral("write_file"), writeUnusedScreen()),
          ScriptedClient::reply(QStringLiteral("done")) });

    registry->remove(QStringLiteral("notes"));

    EXPECT_TRUE(builder->followUp().isEmpty());
}
