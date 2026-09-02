#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <QFile>
#include <QStringList>
#include <QTemporaryDir>
#include <QVariantMap>

#include "agent/agentcontroller.h"
#include "runtime/app.h"
#include "runtime/appregistry.h"
#include "runtime/apptools.h"
#include "scriptedclient.h"

namespace
{
const QString kMainQml = QStringLiteral("import QtQuick\nItem {}\n");

QVariantMap createApp(const QString& id, const QString& title)
{
    return QVariantMap { { QStringLiteral("id"), id },
                         { QStringLiteral("title"), title },
                         { QStringLiteral("qml"), kMainQml } };
}

QVariantMap fileArguments(const QString& app, const QString& path)
{
    return QVariantMap { { QStringLiteral("app"), app }, { QStringLiteral("path"), path } };
}

QString readAll(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}
}

class AppToolsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit());
        registry = std::make_unique<AppRegistry>(nullptr);
        tools = std::make_unique<AppTools>(controller, *registry);
        QObject::connect(tools.get(), &AppTools::appShown,
                         [this](const QString& id) { shown.append(id); });
    }

    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    void run(std::vector<Completion> script)
    {
        client.script = std::move(script);
        controller.request(client, QStringLiteral("go"));
    }

    QVariantMap activity(int index) const { return tools->activities().at(index).toMap(); }

    QTemporaryDir root;
    AgentController controller { QStringLiteral("test-model"), QStringLiteral("system") };
    std::unique_ptr<AppRegistry> registry;
    std::unique_ptr<AppTools> tools;
    ScriptedClient client;
    QStringList shown;
};

TEST_F(AppToolsTest, CreateAppWritesTheEntryFileAndRegistersTheApp)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::reply(QStringLiteral("done")) });

    EXPECT_EQ(readAll(App(QStringLiteral("notes")).entryPath()), kMainQml);
    ASSERT_EQ(registry->apps().size(), 1);
    EXPECT_EQ(registry->apps().first().toMap().value(QStringLiteral("title")).toString(),
              QStringLiteral("Notatki"));
    EXPECT_EQ(shown, QStringList { QStringLiteral("notes") });
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("\"value\":\"ok\"")));
    EXPECT_EQ(controller.lastResponse(), QStringLiteral("done"));
}

TEST_F(AppToolsTest, CreateAppLogsOneBuiltStepNamedAfterTheTitle)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::reply(QStringLiteral("done")) });

    ASSERT_EQ(tools->activities().size(), 1);
    EXPECT_EQ(activity(0).value(QStringLiteral("text")).toString(), QStringLiteral("built"));
    EXPECT_EQ(activity(0).value(QStringLiteral("detail")).toString(), QStringLiteral("Notatki"));
    EXPECT_FALSE(activity(0).value(QStringLiteral("running")).toBool());
    EXPECT_FALSE(activity(0).value(QStringLiteral("failed")).toBool());
}

TEST_F(AppToolsTest, TheStepIsRunningWhileTheToolExecutes)
{
    std::vector<QVariantMap> snapshots;
    QObject::connect(tools.get(), &AppTools::activitiesChanged,
                     [this, &snapshots] { snapshots.push_back(activity(0)); });

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::reply(QStringLiteral("done")) });

    ASSERT_EQ(snapshots.size(), 2u);
    EXPECT_EQ(snapshots[0].value(QStringLiteral("text")).toString(), QStringLiteral("building"));
    EXPECT_TRUE(snapshots[0].value(QStringLiteral("running")).toBool());
    EXPECT_FALSE(snapshots[1].value(QStringLiteral("running")).toBool());
}

TEST_F(AppToolsTest, CreateAppRejectsAnInvalidIdAndMarksTheStepFailed)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("Bad Id"), QStringLiteral("Bad"))),
          ScriptedClient::reply(QStringLiteral("sorry")) });

    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("Invalid app id")));
    EXPECT_TRUE(registry->apps().isEmpty());
    EXPECT_TRUE(shown.isEmpty());
    ASSERT_EQ(tools->activities().size(), 1);
    EXPECT_EQ(activity(0).value(QStringLiteral("text")).toString(),
              QStringLiteral("could not build"));
    EXPECT_TRUE(activity(0).value(QStringLiteral("failed")).toBool());
}

TEST_F(AppToolsTest, FileToolsRefuseAnUnknownAppAndNameTheOnesThatExist)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::toolCall(
              QStringLiteral("read_file"),
              fileArguments(QStringLiteral("timer"), QStringLiteral("Main.qml"))),
          ScriptedClient::reply(QStringLiteral("done")) });

    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("No app \\\"timer\\\"")));
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("notes")));
    ASSERT_EQ(tools->activities().size(), 2);
    EXPECT_EQ(activity(1).value(QStringLiteral("text")).toString(),
              QStringLiteral("could not read"));
    EXPECT_EQ(activity(1).value(QStringLiteral("detail")).toString(), QStringLiteral("Main.qml"));
}

TEST_F(AppToolsTest, WritingAFileShowsTheAppAgainButReadingDoesNot)
{
    QVariantMap write = fileArguments(QStringLiteral("notes"), QStringLiteral("Card.qml"));
    write.insert(QStringLiteral("content"), kMainQml);

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::toolCall(
              QStringLiteral("read_file"),
              fileArguments(QStringLiteral("notes"), QStringLiteral("Main.qml"))),
          ScriptedClient::toolCall(QStringLiteral("write_file"), write),
          ScriptedClient::reply(QStringLiteral("done")) });

    EXPECT_EQ(shown, (QStringList { QStringLiteral("notes"), QStringLiteral("notes") }));
    EXPECT_EQ(readAll(App(QStringLiteral("notes")).dir() + QStringLiteral("/Card.qml")), kMainQml);
    ASSERT_EQ(tools->activities().size(), 3);
    EXPECT_EQ(activity(1).value(QStringLiteral("text")).toString(), QStringLiteral("read"));
    EXPECT_EQ(activity(2).value(QStringLiteral("text")).toString(), QStringLiteral("wrote"));
}

TEST_F(AppToolsTest, ClearingTheLogDropsEveryStep)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::reply(QStringLiteral("done")) });
    int changes = 0;
    QObject::connect(tools.get(), &AppTools::activitiesChanged, [&changes] { ++changes; });

    tools->clearActivities();
    tools->clearActivities();

    EXPECT_TRUE(tools->activities().isEmpty());
    EXPECT_EQ(changes, 1);
}
