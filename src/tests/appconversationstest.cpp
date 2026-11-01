#include <gtest/gtest.h>

#include <QTemporaryDir>

#include "runtime/storage/appconversations.h"

class AppConversationsTest : public ::testing::Test
{
protected:
    void SetUp() override { qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit()); }
    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
};

TEST_F(AppConversationsTest, KnowsNoConversationForAnAppThatWasNeverBound)
{
    AppConversations store;

    EXPECT_TRUE(store.isOpen());
    EXPECT_TRUE(store.conversationFor(QStringLiteral("notes")).isEmpty());
}

TEST_F(AppConversationsTest, ReturnsTheConversationBoundToTheApp)
{
    AppConversations store;

    EXPECT_TRUE(store.bind(QStringLiteral("notes"), QStringLiteral("c-1")));

    EXPECT_EQ(store.conversationFor(QStringLiteral("notes")), QStringLiteral("c-1"));
}

TEST_F(AppConversationsTest, KeepsAppsApart)
{
    AppConversations store;
    store.bind(QStringLiteral("notes"), QStringLiteral("c-1"));
    store.bind(QStringLiteral("timer"), QStringLiteral("c-2"));

    EXPECT_EQ(store.conversationFor(QStringLiteral("notes")), QStringLiteral("c-1"));
    EXPECT_EQ(store.conversationFor(QStringLiteral("timer")), QStringLiteral("c-2"));
}

TEST_F(AppConversationsTest, TheNewestBindingWins)
{
    AppConversations store;
    store.bind(QStringLiteral("notes"), QStringLiteral("c-1"));

    EXPECT_TRUE(store.bind(QStringLiteral("notes"), QStringLiteral("c-2")));

    EXPECT_EQ(store.conversationFor(QStringLiteral("notes")), QStringLiteral("c-2"));
}

TEST_F(AppConversationsTest, FindsTheAppOfAConversation)
{
    AppConversations store;
    store.bind(QStringLiteral("notes"), QStringLiteral("c-1"));
    store.bind(QStringLiteral("timer"), QStringLiteral("c-2"));

    EXPECT_EQ(store.appFor(QStringLiteral("c-1")), QStringLiteral("notes"));
    EXPECT_EQ(store.appFor(QStringLiteral("c-2")), QStringLiteral("timer"));
}

TEST_F(AppConversationsTest, KnowsNoAppForAConversationThatWasNeverBound)
{
    AppConversations store;
    store.bind(QStringLiteral("notes"), QStringLiteral("c-1"));

    EXPECT_TRUE(store.appFor(QStringLiteral("c-9")).isEmpty());
}

TEST_F(AppConversationsTest, ReboundAppLeavesItsOldConversationWithoutAnApp)
{
    AppConversations store;
    store.bind(QStringLiteral("notes"), QStringLiteral("c-1"));
    store.bind(QStringLiteral("notes"), QStringLiteral("c-2"));

    EXPECT_TRUE(store.appFor(QStringLiteral("c-1")).isEmpty());
    EXPECT_EQ(store.appFor(QStringLiteral("c-2")), QStringLiteral("notes"));
}

TEST_F(AppConversationsTest, TheNewestAppWinsWhenOneConversationIsBoundTwice)
{
    AppConversations store;
    store.bind(QStringLiteral("notes"), QStringLiteral("c-1"));
    store.bind(QStringLiteral("timer"), QStringLiteral("c-1"));

    EXPECT_EQ(store.appFor(QStringLiteral("c-1")), QStringLiteral("timer"));
}

TEST_F(AppConversationsTest, RemoveForgetsOnlyThatApp)
{
    AppConversations store;
    store.bind(QStringLiteral("notes"), QStringLiteral("c-1"));
    store.bind(QStringLiteral("timer"), QStringLiteral("c-2"));

    EXPECT_TRUE(store.remove(QStringLiteral("notes")));

    EXPECT_TRUE(store.conversationFor(QStringLiteral("notes")).isEmpty());
    EXPECT_TRUE(store.appFor(QStringLiteral("c-1")).isEmpty());
    EXPECT_EQ(store.conversationFor(QStringLiteral("timer")), QStringLiteral("c-2"));
}

TEST_F(AppConversationsTest, BindingsSurviveReopeningTheStore)
{
    {
        AppConversations store;
        store.bind(QStringLiteral("notes"), QStringLiteral("c-1"));
    }

    AppConversations reopened;

    EXPECT_EQ(reopened.conversationFor(QStringLiteral("notes")), QStringLiteral("c-1"));
}
