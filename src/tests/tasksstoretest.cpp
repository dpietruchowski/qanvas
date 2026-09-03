#include <gtest/gtest.h>

#include <QStringList>
#include <QTemporaryDir>
#include <QVariantMap>

#include "runtime/storage/tasksstore.h"

namespace
{
QStringList texts(const QVariantList& tasks)
{
    QStringList out;
    for (const QVariant& task : tasks)
        out.append(task.toMap().value(QStringLiteral("text")).toString());
    return out;
}
}

class TasksStoreTest : public ::testing::Test
{
protected:
    void SetUp() override { qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit()); }
    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
};

TEST_F(TasksStoreTest, AddsOpenTasksAndListsTheNewestFirst)
{
    TasksStore store;

    const qint64 first = store.add(QStringLiteral("call mum"));
    const qint64 second = store.add(QStringLiteral("  buy milk  "));

    EXPECT_GT(first, 0);
    EXPECT_GT(second, first);
    EXPECT_EQ(texts(store.list()),
              (QStringList { QStringLiteral("buy milk"), QStringLiteral("call mum") }));
    const QVariantMap task = store.list().first().toMap();
    EXPECT_EQ(task.value(QStringLiteral("id")).toLongLong(), second);
    EXPECT_FALSE(task.value(QStringLiteral("done")).toBool());
    EXPECT_FALSE(task.value(QStringLiteral("createdAt")).toString().isEmpty());
    EXPECT_FALSE(task.value(QStringLiteral("updatedAt")).toString().isEmpty());
}

TEST_F(TasksStoreTest, RefusesABlankTask)
{
    TasksStore store;

    EXPECT_EQ(store.add(QStringLiteral("   ")), -1);
    EXPECT_TRUE(store.list().isEmpty());
}

TEST_F(TasksStoreTest, DoneTasksSinkBelowTheOpenOnes)
{
    TasksStore store;
    const qint64 milk = store.add(QStringLiteral("buy milk"));
    store.add(QStringLiteral("call mum"));

    EXPECT_TRUE(store.setDone(milk, true));

    EXPECT_EQ(texts(store.list()),
              (QStringList { QStringLiteral("call mum"), QStringLiteral("buy milk") }));
    EXPECT_TRUE(store.list().last().toMap().value(QStringLiteral("done")).toBool());

    EXPECT_TRUE(store.setDone(milk, false));

    EXPECT_FALSE(store.list().first().toMap().value(QStringLiteral("done")).toBool());
    EXPECT_FALSE(store.setDone(milk + 10, true));
}

TEST_F(TasksStoreTest, UpdatesAndRemovesByIdOnly)
{
    TasksStore store;
    const qint64 id = store.add(QStringLiteral("buy milk"));

    EXPECT_TRUE(store.update(id, QStringLiteral("buy oat milk")));
    EXPECT_FALSE(store.update(id, QStringLiteral(" ")));
    EXPECT_FALSE(store.update(id + 1, QStringLiteral("nothing")));
    EXPECT_EQ(texts(store.list()), QStringList { QStringLiteral("buy oat milk") });

    EXPECT_FALSE(store.remove(id + 1));
    EXPECT_TRUE(store.remove(id));
    EXPECT_TRUE(store.list().isEmpty());
}

TEST_F(TasksStoreTest, SurvivesAcrossInstances)
{
    TasksStore().add(QStringLiteral("buy milk"));

    EXPECT_EQ(texts(TasksStore().list()), QStringList { QStringLiteral("buy milk") });
}
