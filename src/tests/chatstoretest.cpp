#include <gtest/gtest.h>

#include <QStringList>
#include <QTemporaryDir>
#include <QVariantList>
#include <QVariantMap>

#include "runtime/storage/chatstore.h"

namespace
{
QStringList texts(const QVariantList& messages)
{
    QStringList out;
    for (const QVariant& message : messages)
        out.append(message.toMap().value(QStringLiteral("sender")).toString() + QLatin1Char(':')
                   + message.toMap().value(QStringLiteral("text")).toString());
    return out;
}
}

class ChatStoreTest : public ::testing::Test
{
protected:
    void SetUp() override { qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit()); }
    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
};

TEST_F(ChatStoreTest, ListsWhatWasAppendedInOrder)
{
    ChatStore store;

    EXPECT_TRUE(store.append(QStringLiteral("notes"), QStringLiteral("user"), QStringLiteral("a")));
    EXPECT_TRUE(
        store.append(QStringLiteral("notes"), QStringLiteral("assistant"), QStringLiteral("b")));

    EXPECT_EQ(texts(store.messages(QStringLiteral("notes"))),
              (QStringList { QStringLiteral("user:a"), QStringLiteral("assistant:b") }));
}

TEST_F(ChatStoreTest, KeepsAppsApartAndTreatsTheEmptyIdAsAnApp)
{
    ChatStore store;
    store.append(QStringLiteral("notes"), QStringLiteral("user"), QStringLiteral("a"));
    store.append(QString(), QStringLiteral("user"), QStringLiteral("scratch"));

    EXPECT_EQ(texts(store.messages(QStringLiteral("notes"))),
              QStringList { QStringLiteral("user:a") });
    EXPECT_EQ(texts(store.messages(QString())), QStringList { QStringLiteral("user:scratch") });
    EXPECT_TRUE(store.messages(QStringLiteral("timer")).isEmpty());
}

TEST_F(ChatStoreTest, RebindMovesEveryMessageOfAThread)
{
    ChatStore store;
    store.append(QString(), QStringLiteral("user"), QStringLiteral("a"));
    store.append(QString(), QStringLiteral("assistant"), QStringLiteral("b"));
    store.append(QStringLiteral("notes"), QStringLiteral("user"), QStringLiteral("c"));

    EXPECT_TRUE(store.rebind(QString(), QStringLiteral("notes")));

    EXPECT_TRUE(store.messages(QString()).isEmpty());
    EXPECT_EQ(texts(store.messages(QStringLiteral("notes"))),
              (QStringList { QStringLiteral("user:a"), QStringLiteral("assistant:b"),
                             QStringLiteral("user:c") }));
}

TEST_F(ChatStoreTest, RemoveDropsOneThread)
{
    ChatStore store;
    store.append(QStringLiteral("notes"), QStringLiteral("user"), QStringLiteral("a"));
    store.append(QStringLiteral("timer"), QStringLiteral("user"), QStringLiteral("b"));

    EXPECT_TRUE(store.remove(QStringLiteral("notes")));

    EXPECT_TRUE(store.messages(QStringLiteral("notes")).isEmpty());
    EXPECT_EQ(store.messages(QStringLiteral("timer")).size(), 1);
}

TEST_F(ChatStoreTest, SurvivesAcrossInstances)
{
    {
        ChatStore store;
        store.append(QStringLiteral("notes"), QStringLiteral("user"), QStringLiteral("a"));
    }

    ChatStore reopened;

    EXPECT_EQ(texts(reopened.messages(QStringLiteral("notes"))),
              QStringList { QStringLiteral("user:a") });
}
