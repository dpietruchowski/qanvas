#include <gtest/gtest.h>

#include <memory>

#include <QTemporaryDir>
#include <QVariantMap>

#include "runtime/app/appregistry.h"
#include "runtime/host/notesbridge.h"
#include "runtime/host/tasksbridge.h"
#include "runtime/storage/tasksstore.h"

class TasksBridgeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit());
        registry = std::make_unique<AppRegistry>(nullptr);
    }

    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
    std::unique_ptr<AppRegistry> registry;
};

TEST_F(TasksBridgeTest, RefusesEverythingWithoutTheCapability)
{
    TasksStore().add(QStringLiteral("secret"));
    TasksBridge bridge(QStringLiteral("spy"), *registry);

    EXPECT_FALSE(bridge.allowed());
    EXPECT_TRUE(bridge.list().isEmpty());
    EXPECT_FALSE(bridge.add(QStringLiteral("mine")));
    EXPECT_FALSE(bridge.setDone(1, true));
    EXPECT_FALSE(bridge.remove(1));
    EXPECT_EQ(bridge.lastError(), QStringLiteral("\"spy\" may not use the user's tasks"));
    EXPECT_EQ(bridge.revision(), 0);
    EXPECT_EQ(TasksStore().list().size(), 1);
}

TEST_F(TasksBridgeTest, ReadsWritesAndMarksDoneOnceGranted)
{
    registry->grant(QStringLiteral("todo"), TasksBridge::capability());
    TasksBridge bridge(QStringLiteral("todo"), *registry);
    int changes = 0;
    QObject::connect(&bridge, &TasksBridge::changed, [&changes] { ++changes; });

    EXPECT_TRUE(bridge.allowed());
    EXPECT_TRUE(bridge.add(QStringLiteral("buy milk")));
    ASSERT_EQ(bridge.list().size(), 1);
    const int id = bridge.list().first().toMap().value(QStringLiteral("id")).toInt();
    EXPECT_TRUE(bridge.update(id, QStringLiteral("buy oat milk")));
    EXPECT_TRUE(bridge.setDone(id, true));
    EXPECT_TRUE(bridge.list().first().toMap().value(QStringLiteral("done")).toBool());
    EXPECT_TRUE(bridge.remove(id));
    EXPECT_TRUE(bridge.list().isEmpty());
    EXPECT_EQ(bridge.revision(), 4);
    EXPECT_EQ(changes, 4);
    EXPECT_TRUE(bridge.lastError().isEmpty());
}

TEST_F(TasksBridgeTest, AFailedWriteExplainsItself)
{
    registry->grant(QStringLiteral("todo"), TasksBridge::capability());
    TasksBridge bridge(QStringLiteral("todo"), *registry);

    EXPECT_FALSE(bridge.add(QStringLiteral("  ")));
    EXPECT_EQ(bridge.lastError(), QStringLiteral("A task needs some text"));
    EXPECT_FALSE(bridge.setDone(42, true));
    EXPECT_EQ(bridge.lastError(), QStringLiteral("No task 42 to mark done"));
    EXPECT_FALSE(bridge.remove(42));
    EXPECT_EQ(bridge.lastError(), QStringLiteral("No task 42 to remove"));
    EXPECT_EQ(bridge.revision(), 0);
}

TEST_F(TasksBridgeTest, AGrantForNotesDoesNotOpenTheTasks)
{
    registry->grant(QStringLiteral("notes"), NotesBridge::capability());
    NotesBridge notes(QStringLiteral("notes"), *registry);
    TasksBridge tasks(QStringLiteral("notes"), *registry);

    EXPECT_TRUE(notes.allowed());
    EXPECT_FALSE(tasks.allowed());
}
