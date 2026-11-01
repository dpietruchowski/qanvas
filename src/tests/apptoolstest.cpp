#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include <QFile>
#include <QMap>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QStringList>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariantMap>

#include "agent/agentcontroller.h"
#include "runtime/app/app.h"
#include "runtime/app/appregistry.h"
#include "runtime/chat/apptools.h"
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
        tools = std::make_unique<AppTools>(
            [this](const QString& docstring, const Tool& tool, ToolEffect effect)
            {
                registered.push_back({ docstring, effect });
                controller.addTool(docstring, tool);
            },
            *registry);
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
    std::vector<std::pair<QString, ToolEffect>> registered;
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

TEST_F(AppToolsTest, CreateAppWritesTheDataComponentBesideTheEntryFile)
{
    QVariantMap arguments = createApp(QStringLiteral("notes"), QStringLiteral("Notatki"));
    arguments[QStringLiteral("qml")] = QStringLiteral("import QtQuick\nItem { Data { id: db } }\n");
    arguments[QStringLiteral("data")]
        = QStringLiteral("import QtQuick\nQtObject { function setup() {} }\n");

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), arguments),
          ScriptedClient::reply(QStringLiteral("done")) });

    EXPECT_EQ(readAll(App(QStringLiteral("notes")).dir() + QStringLiteral("/Data.qml")),
              arguments.value(QStringLiteral("data")).toString());
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("\"value\":\"ok\"")));
    EXPECT_FALSE(activity(0).value(QStringLiteral("failed")).toBool());
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

TEST_F(AppToolsTest, FileToolsRefuseAnUnknownAppWhenNoneIsOpen)
{
    run({ ScriptedClient::toolCall(
              QStringLiteral("read_file"),
              fileArguments(QStringLiteral("timer"), QStringLiteral("Main.qml"))),
          ScriptedClient::reply(QStringLiteral("done")) });

    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("No app \\\"timer\\\"")));
    ASSERT_EQ(tools->activities().size(), 1);
    EXPECT_EQ(activity(0).value(QStringLiteral("text")).toString(),
              QStringLiteral("could not read"));
    EXPECT_EQ(activity(0).value(QStringLiteral("detail")).toString(), QStringLiteral("Main.qml"));
}

TEST_F(AppToolsTest, AWrongAppIdGoesToTheOpenAppWithANote)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::toolCall(
              QStringLiteral("read_file"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notatki") },
                            { QStringLiteral("path"), QStringLiteral("Main.qml") },
                            { QStringLiteral("from"), 0 },
                            { QStringLiteral("lines"), 0 } }),
          ScriptedClient::reply(QStringLiteral("done")) });

    const QString result = client.lastToolResult();
    EXPECT_TRUE(result.contains(QStringLiteral("the open app is \\\"notes\\\"")))
        << qPrintable(result);
    EXPECT_TRUE(result.contains(QStringLiteral("import QtQuick"))) << qPrintable(result);
    EXPECT_FALSE(activity(1).value(QStringLiteral("failed")).toBool());
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

TEST_F(AppToolsTest, AWriteThatAddsNoNewErrorIsKeptWhileTheAppIsStillBroken)
{
    QVariantMap arguments = createApp(QStringLiteral("notes"), QStringLiteral("Notatki"));
    arguments[QStringLiteral("qml")]
        = QStringLiteral("import QtQuick\nItem { Card {} Panel {} }\n");
    QVariantMap card = fileArguments(QStringLiteral("notes"), QStringLiteral("Card.qml"));
    card[QStringLiteral("content")] = kMainQml;

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), arguments),
          ScriptedClient::toolCall(QStringLiteral("write_file"), card),
          ScriptedClient::reply(QStringLiteral("half")) });

    const QString kept = client.lastToolResult();
    EXPECT_TRUE(kept.contains(QStringLiteral("Saved, but"))) << qPrintable(kept);
    EXPECT_TRUE(kept.contains(QStringLiteral("no Panel.qml in the app"))) << qPrintable(kept);
    EXPECT_FALSE(kept.contains(QStringLiteral("undone"))) << qPrintable(kept);
    EXPECT_TRUE(QFile::exists(App(QStringLiteral("notes")).dir() + QStringLiteral("/Card.qml")));
    EXPECT_FALSE(activity(1).value(QStringLiteral("failed")).toBool());
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

TEST_F(AppToolsTest, ReadLogNamesTheFilesNoOtherFileUses)
{
    QVariantMap card = fileArguments(QStringLiteral("notes"), QStringLiteral("Card.qml"));
    card[QStringLiteral("content")] = kMainQml;

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::toolCall(QStringLiteral("write_file"), card),
          ScriptedClient::toolCall(
              QStringLiteral("read_log"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("done")) });

    const QString result = client.lastToolResult();
    EXPECT_TRUE(result.contains(QStringLiteral("No errors"))) << qPrintable(result);
    EXPECT_TRUE(result.contains(QStringLiteral("cannot reach them: Card.qml")))
        << qPrintable(result);
}

TEST_F(AppToolsTest, AWriteThatCallsAFunctionDataDoesNotDefineSaysSo)
{
    QVariantMap arguments = createApp(QStringLiteral("notes"), QStringLiteral("Notatki"));
    arguments[QStringLiteral("qml")] = QStringLiteral("import QtQuick\nItem { Data { id: db } }\n");
    arguments[QStringLiteral("data")]
        = QStringLiteral("import QtQuick\nQtObject {\n    function setup() {}\n}\n");
    QVariantMap screen = fileArguments(QStringLiteral("notes"), QStringLiteral("Stats.qml"));
    screen[QStringLiteral("content")] = QStringLiteral("import QtQuick\nItem {\n    Data { id: db "
                                                       "}\n    property var s: db.stats()\n}\n");

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"), arguments),
          ScriptedClient::toolCall(QStringLiteral("write_file"), screen),
          ScriptedClient::toolCall(
              QStringLiteral("read_log"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("done")) });

    const QString written = client.toolResults().at(1);
    EXPECT_TRUE(written.contains(QStringLiteral("Stats.qml:4: calls db.stats(), which Data.qml "
                                                "does not define (it has: setup)")))
        << qPrintable(written);
    EXPECT_TRUE(written.contains(QStringLiteral("Add the function to Data.qml")))
        << qPrintable(written);
    EXPECT_FALSE(activity(1).value(QStringLiteral("failed")).toBool());
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("Will throw when that screen")))
        << qPrintable(client.lastToolResult());
}

TEST_F(AppToolsTest, ReadLogNamesARejectedWriteThatWasNeverRedone)
{
    QVariantMap card = fileArguments(QStringLiteral("notes"), QStringLiteral("Card.qml"));
    card[QStringLiteral("content")] = QStringLiteral("import QtQuick\nItem {\n");

    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::toolCall(QStringLiteral("write_file"), card),
          ScriptedClient::toolCall(
              QStringLiteral("read_log"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("done")) });

    const QString result = client.lastToolResult();
    EXPECT_TRUE(result.contains(QStringLiteral("never redone"))
                && result.contains(QStringLiteral("Card.qml")))
        << qPrintable(result);
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
    EXPECT_EQ(activity(1).value(QStringLiteral("text")).toString(), QStringLiteral("shared"));
    EXPECT_EQ(activity(1).value(QStringLiteral("detail")).toString(), QStringLiteral("notes"));
}

TEST_F(AppToolsTest, GrantAccessSharesTheTasksToo)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("todo"), QStringLiteral("Zadania"))),
          ScriptedClient::toolCall(
              QStringLiteral("grant_access"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("todo") },
                            { QStringLiteral("store"), QStringLiteral("tasks") } }),
          ScriptedClient::reply(QStringLiteral("shared")) });

    EXPECT_TRUE(registry->has(QStringLiteral("todo"), QStringLiteral("tasks")));
    EXPECT_FALSE(registry->has(QStringLiteral("todo"), QStringLiteral("notes")));
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("may use Tasks")));
    EXPECT_EQ(activity(1).value(QStringLiteral("detail")).toString(), QStringLiteral("tasks"));
}

TEST_F(AppToolsTest, GrantAccessKnowsOnlyTheSharedStores)
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
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("notes or tasks")));
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

TEST_F(AppToolsTest, DescribeAppListsFunctionsAndTables)
{
    run({ ScriptedClient::toolCall(
              QStringLiteral("create_app"),
              QVariantMap { { QStringLiteral("id"), QStringLiteral("notes") },
                            { QStringLiteral("title"), QStringLiteral("Notatki") },
                            { QStringLiteral("qml"),
                              QStringLiteral("import QtQuick\nItem {\n    function greet(name) { "
                                             "return name }\n}\n") } }),
          ScriptedClient::toolCall(
              QStringLiteral("describe_app"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("seen")) });

    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("Main.qml 4L")));
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("greet(name)")));
    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("No database yet")));
    EXPECT_EQ(shown.size(), 1);
    EXPECT_EQ(activity(1).value(QStringLiteral("text")).toString(), QStringLiteral("inspected"));
    EXPECT_EQ(activity(1).value(QStringLiteral("detail")).toString(), QStringLiteral("notes"));
}

TEST_F(AppToolsTest, DescribeAppRefusesAnAppThatIsNotThere)
{
    run({ ScriptedClient::toolCall(
              QStringLiteral("describe_app"),
              QVariantMap { { QStringLiteral("app"), QStringLiteral("notes") } }),
          ScriptedClient::reply(QStringLiteral("sorry")) });

    EXPECT_TRUE(client.lastToolResult().contains(QStringLiteral("No app")));
    EXPECT_TRUE(activity(0).value(QStringLiteral("failed")).toBool());
}

TEST_F(AppToolsTest, RegistersEveryToolThroughTheRegistrarWithItsEffect)
{
    QMap<QString, ToolEffect> effects;
    for (const auto& [docstring, effect] : registered)
    {
        const QString name
            = docstring.section(QLatin1Char('\n'), 0, 0).mid(QStringLiteral("@brief ").size());
        effects.insert(name, effect);
    }

    const QStringList reads { "list_files",   "read_file", "search_files",
                              "describe_app", "read_log",  "check_layout" };
    const QStringList writes { "create_app",  "write_file", "edit_file",
                               "delete_file", "delete_app", "grant_access" };
    EXPECT_EQ(effects.size(), reads.size() + writes.size());
    for (const QString& name : reads)
        EXPECT_EQ(effects.value(name, ToolEffect::Writes), ToolEffect::Reads) << qPrintable(name);
    for (const QString& name : writes)
        EXPECT_EQ(effects.value(name, ToolEffect::Reads), ToolEffect::Writes) << qPrintable(name);
}

TEST_F(AppToolsTest, ClearActivitiesForgetsTheTurnState)
{
    run({ ScriptedClient::toolCall(QStringLiteral("create_app"),
                                   createApp(QStringLiteral("notes"), QStringLiteral("Notatki"))),
          ScriptedClient::reply(QStringLiteral("done")) });
    ASSERT_FALSE(tools->activities().isEmpty());

    tools->clearActivities();

    EXPECT_TRUE(tools->activities().isEmpty());
    EXPECT_TRUE(tools->unfinished(App(QStringLiteral("notes")), true).isEmpty());
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
