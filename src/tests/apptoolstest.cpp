#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QStringList>
#include <QTemporaryDir>
#include <QUrl>
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
        engine.setOutputWarningsToStandardError(false);
        tools = std::make_unique<AppTools>(controller, *registry);
        tools->setEngine(&engine);
        QObject::connect(tools.get(), &AppTools::appShown,
                         [this](const QString& id)
                         {
                             shown.append(id);
                             load(id);
                         });
    }

    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    void load(const QString& id)
    {
        running.reset();
        QQmlComponent component(&engine, QUrl::fromLocalFile(App(id).entryPath()));
        running.reset(component.create());
    }

    void run(std::vector<Completion> script)
    {
        client.script = std::move(script);
        requestAndWait(controller, client, QStringLiteral("go"));
    }

    QVariantMap activity(int index) const { return tools->activities().at(index).toMap(); }

    QTemporaryDir root;
    QQmlEngine engine;
    AgentController controller { QStringLiteral("test-model"), QStringLiteral("system") };
    std::unique_ptr<AppRegistry> registry;
    std::unique_ptr<AppTools> tools;
    std::unique_ptr<QObject> running;
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

TEST_F(AppToolsTest, CreateAppWithQmlThatDoesNotLoadTellsTheModelWhereAndKeepsTheApp)
{
    QVariantMap arguments = createApp(QStringLiteral("notes"), QStringLiteral("Notatki"));
    arguments[QStringLiteral("qml")] = QStringLiteral("import QtQuick\nItem {\n");

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), arguments),
          ScriptedClient::reply(QStringLiteral("sorry")) });

    const QString result = client.lastToolResult();
    EXPECT_TRUE(result.contains(QStringLiteral("does not load"))) << qPrintable(result);
    EXPECT_TRUE(result.contains(QStringLiteral("Main.qml:3"))) << qPrintable(result);
    EXPECT_TRUE(App(QStringLiteral("notes")).exists());
    EXPECT_EQ(registry->apps().size(), 1);
    EXPECT_EQ(shown, QStringList { QStringLiteral("notes") });
    EXPECT_EQ(activity(0).value(QStringLiteral("text")).toString(),
              QStringLiteral("could not build"));
    EXPECT_TRUE(activity(0).value(QStringLiteral("failed")).toBool());
}

TEST_F(AppToolsTest, EditingTheFileIntoShapeReportsSuccess)
{
    QVariantMap broken = createApp(QStringLiteral("notes"), QStringLiteral("Notatki"));
    broken[QStringLiteral("qml")] = QStringLiteral("import QtQuick\nItem {\n");
    QVariantMap edit = fileArguments(QStringLiteral("notes"), QStringLiteral("Main.qml"));
    edit[QStringLiteral("before")] = QStringLiteral("Item {\n");
    edit[QStringLiteral("after")] = QStringLiteral("Item {}\n");

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), broken),
          ScriptedClient::toolCall(QStringLiteral("edit_file"), edit),
          ScriptedClient::reply(QStringLiteral("fixed")) });

    const QString result = client.lastToolResult();
    EXPECT_FALSE(result.contains(QStringLiteral("does not load"))) << qPrintable(result);
    ASSERT_EQ(tools->activities().size(), 2);
    EXPECT_EQ(activity(1).value(QStringLiteral("text")).toString(), QStringLiteral("edited"));
    EXPECT_FALSE(activity(1).value(QStringLiteral("failed")).toBool());
    EXPECT_EQ(shown, (QStringList { QStringLiteral("notes"), QStringLiteral("notes") }));
}

TEST_F(AppToolsTest, WritingTheMissingComponentMakesTheAppLoad)
{
    QVariantMap arguments = createApp(QStringLiteral("notes"), QStringLiteral("Notatki"));
    arguments[QStringLiteral("qml")] = QStringLiteral("import QtQuick\nItem { Card {} }\n");
    QVariantMap write = fileArguments(QStringLiteral("notes"), QStringLiteral("Card.qml"));
    write[QStringLiteral("content")] = kMainQml;

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), arguments),
          ScriptedClient::toolCall(QStringLiteral("write_file"), write),
          ScriptedClient::reply(QStringLiteral("done")) });

    ASSERT_EQ(tools->activities().size(), 2);
    EXPECT_TRUE(activity(0).value(QStringLiteral("failed")).toBool());
    EXPECT_FALSE(activity(1).value(QStringLiteral("failed")).toBool());
    EXPECT_FALSE(client.lastToolResult().contains(QStringLiteral("does not load")));
}

TEST_F(AppToolsTest, AnErrorRaisedWhileLoadingComesBackWithTheWriteResult)
{
    QVariantMap arguments = createApp(QStringLiteral("notes"), QStringLiteral("Notatki"));
    arguments[QStringLiteral("qml")]
        = QStringLiteral("import QtQuick\nItem {\n    Component.onCompleted: undefined.foo\n}\n");

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), arguments),
          ScriptedClient::reply(QStringLiteral("sorry")) });

    const QString result = client.lastToolResult();
    EXPECT_TRUE(result.contains(QStringLiteral("the app raised"))) << qPrintable(result);
    EXPECT_TRUE(result.contains(QStringLiteral("Main.qml:3"))) << qPrintable(result);
    EXPECT_TRUE(result.contains(QStringLiteral("TypeError"))) << qPrintable(result);
    EXPECT_TRUE(activity(0).value(QStringLiteral("failed")).toBool());
}

TEST_F(AppToolsTest, ReadLogListsTheErrorsRaisedSinceTheLastLoad)
{
    QVariantMap arguments = createApp(QStringLiteral("notes"), QStringLiteral("Notatki"));
    arguments[QStringLiteral("qml")]
        = QStringLiteral("import QtQuick\nItem {\n    Component.onCompleted: undefined.foo\n}\n");

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), arguments),
          ScriptedClient::toolCall(
              QStringLiteral("read_log"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("done")) });

    const QString result = client.lastToolResult();
    EXPECT_TRUE(result.contains(QStringLiteral("TypeError"))) << qPrintable(result);
    ASSERT_EQ(tools->activities().size(), 2);
    EXPECT_EQ(activity(1).value(QStringLiteral("text")).toString(), QStringLiteral("checked"));
    EXPECT_EQ(activity(1).value(QStringLiteral("detail")).toString(), QStringLiteral("notes"));
}

TEST_F(AppToolsTest, ReadLogSaysSoWhenTheAppRaisedNothing)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::toolCall(
              QStringLiteral("read_log"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("done")) });

    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("No errors")));
}

TEST_F(AppToolsTest, FixingTheErrorClearsTheLog)
{
    QVariantMap arguments = createApp(QStringLiteral("notes"), QStringLiteral("Notatki"));
    arguments[QStringLiteral("qml")]
        = QStringLiteral("import QtQuick\nItem {\n    Component.onCompleted: undefined.foo\n}\n");
    QVariantMap edit = fileArguments(QStringLiteral("notes"), QStringLiteral("Main.qml"));
    edit[QStringLiteral("before")] = QStringLiteral("undefined.foo");
    edit[QStringLiteral("after")] = QStringLiteral("console.log(1)");

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), arguments),
          ScriptedClient::toolCall(QStringLiteral("edit_file"), edit),
          ScriptedClient::toolCall(
              QStringLiteral("read_log"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("done")) });

    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("No errors")));
    ASSERT_EQ(tools->activities().size(), 3);
    EXPECT_FALSE(activity(1).value(QStringLiteral("failed")).toBool());
}

TEST_F(AppToolsTest, DeleteAppRemovesTheFilesAndTheListEntry)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::toolCall(
              QStringLiteral("delete_app"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("gone")) });

    EXPECT_FALSE(App(QStringLiteral("notes")).exists());
    EXPECT_TRUE(registry->apps().isEmpty());
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("Deleted")));
    ASSERT_EQ(tools->activities().size(), 2);
    EXPECT_EQ(activity(1).value(QStringLiteral("text")).toString(), QStringLiteral("deleted"));
    EXPECT_EQ(activity(1).value(QStringLiteral("detail")).toString(), QStringLiteral("notes"));
    EXPECT_FALSE(activity(1).value(QStringLiteral("failed")).toBool());
}

TEST_F(AppToolsTest, GrantAccessRecordsThePermissionAndShowsTheAppAgain)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::toolCall(
              QStringLiteral("grant_access"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") },
                            { QStringLiteral("store"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("shared")) });

    EXPECT_TRUE(registry->has(QStringLiteral("notes"), QStringLiteral("notes")));
    EXPECT_EQ(shown, (QStringList { QStringLiteral("notes"), QStringLiteral("notes") }));
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("may use Notes")));
    ASSERT_EQ(tools->activities().size(), 2);
    EXPECT_EQ(activity(1).value(QStringLiteral("text")).toString(),
              QStringLiteral("shared notes with"));
    EXPECT_EQ(activity(1).value(QStringLiteral("detail")).toString(), QStringLiteral("notes"));
}

TEST_F(AppToolsTest, GrantAccessKnowsOnlyTheNotesStore)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::toolCall(
              QStringLiteral("grant_access"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") },
                            { QStringLiteral("store"), QStringLiteral("contacts") } }),
          ScriptedClient::reply(QStringLiteral("sorry")) });

    EXPECT_TRUE(registry->capabilities(QStringLiteral("notes")).isEmpty());
    EXPECT_EQ(shown.size(), 1);
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("No store")));
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("contacts")));
    EXPECT_TRUE(activity(1).value(QStringLiteral("failed")).toBool());
}

TEST_F(AppToolsTest, GrantAccessRefusesAnAppThatIsNotThere)
{
    run({ ScriptedClient::toolCall(
              QStringLiteral("grant_access"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") },
                            { QStringLiteral("store"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("sorry")) });

    EXPECT_TRUE(registry->capabilities(QStringLiteral("notes")).isEmpty());
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("No app")));
    EXPECT_TRUE(activity(0).value(QStringLiteral("failed")).toBool());
}

TEST_F(AppToolsTest, DeleteAppRefusesAnAppThatIsNotThere)
{
    run({ ScriptedClient::toolCall(
              QStringLiteral("delete_app"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("sorry")) });

    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("No app")));
    EXPECT_TRUE(activity(0).value(QStringLiteral("failed")).toBool());
}
