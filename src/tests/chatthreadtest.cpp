#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <QStringList>
#include <QTemporaryDir>
#include <QVariantList>
#include <QVariantMap>

#include "agent/agentcontroller.h"
#include "runtime/chat/chatthread.h"
#include "runtime/storage/chatstore.h"
#include "scriptedclient.h"

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

class ChatThreadTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit());
        store = std::make_unique<ChatStore>();
        thread = std::make_unique<ChatThread>(controller, *store);
        QObject::connect(thread.get(), &ChatThread::appChanged, [this] { ++appChanges; });
    }

    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    void talk(const QString& text, const QString& reply)
    {
        client.script = { ScriptedClient::reply(reply) };
        client.calls = 0;
        thread->remember(text);
        requestAndWait(controller, client, text);
    }

    QTemporaryDir root;
    AgentController controller { QStringLiteral("test-model"), QStringLiteral("system") };
    std::unique_ptr<ChatStore> store;
    std::unique_ptr<ChatThread> thread;
    ScriptedClient client;
    int appChanges = 0;
};

TEST_F(ChatThreadTest, RemembersTheUserAndTheReplyUnderTheCurrentApp)
{
    talk(QStringLiteral("hi"), QStringLiteral("hello"));

    EXPECT_EQ(thread->app(), QString());
    EXPECT_EQ(texts(store->messages(QString())),
              (QStringList { QStringLiteral("user:hi"), QStringLiteral("assistant:hello") }));
}

TEST_F(ChatThreadTest, MovesAScratchThreadToTheFirstAppTheAgentShows)
{
    thread->remember(QStringLiteral("make notes"));

    thread->bind(QStringLiteral("notes"));
    talk(QStringLiteral("thanks"), QStringLiteral("welcome"));

    EXPECT_EQ(thread->app(), QStringLiteral("notes"));
    EXPECT_EQ(appChanges, 1);
    EXPECT_TRUE(store->messages(QString()).isEmpty());
    EXPECT_EQ(texts(store->messages(QStringLiteral("notes"))),
              (QStringList { QStringLiteral("user:make notes"), QStringLiteral("user:thanks"),
                             QStringLiteral("assistant:welcome") }));
}

TEST_F(ChatThreadTest, AnotherAppShownLaterStartsItsOwnThread)
{
    thread->bind(QStringLiteral("notes"));
    thread->remember(QStringLiteral("about notes"));

    thread->bind(QStringLiteral("timer"));
    thread->remember(QStringLiteral("about timer"));

    EXPECT_EQ(thread->app(), QStringLiteral("timer"));
    EXPECT_EQ(texts(store->messages(QStringLiteral("notes"))),
              QStringList { QStringLiteral("user:about notes") });
    EXPECT_EQ(texts(store->messages(QStringLiteral("timer"))),
              QStringList { QStringLiteral("user:about timer") });
}

TEST_F(ChatThreadTest, ShowingTheCurrentAppAgainChangesNothing)
{
    thread->bind(QStringLiteral("notes"));

    thread->bind(QStringLiteral("notes"));

    EXPECT_EQ(appChanges, 1);
}

TEST_F(ChatThreadTest, OpeningAnAppRestoresItsConversationAndSendsItToTheModel)
{
    store->append(QStringLiteral("notes"), QStringLiteral("user"), QStringLiteral("a"));
    store->append(QStringLiteral("notes"), QStringLiteral("assistant"), QStringLiteral("b"));

    thread->open(QStringLiteral("notes"));

    EXPECT_EQ(thread->app(), QStringLiteral("notes"));
    EXPECT_EQ(texts(controller.messages()),
              (QStringList { QStringLiteral("user:a"), QStringLiteral("assistant:b") }));

    talk(QStringLiteral("c"), QStringLiteral("d"));

    ASSERT_EQ(client.lastMessages.size(), 4);
    EXPECT_EQ(client.lastMessages[1].content, QStringLiteral("a"));
    EXPECT_EQ(client.lastMessages[2].content, QStringLiteral("b"));
    EXPECT_EQ(client.lastMessages[3].content, QStringLiteral("c"));
    EXPECT_EQ(texts(store->messages(QStringLiteral("notes"))),
              (QStringList { QStringLiteral("user:a"), QStringLiteral("assistant:b"),
                             QStringLiteral("user:c"), QStringLiteral("assistant:d") }));
}

TEST_F(ChatThreadTest, ForgettingTheCurrentAppDropsItsHistoryAndGoesBackToScratch)
{
    thread->bind(QStringLiteral("notes"));
    thread->remember(QStringLiteral("x"));

    thread->forget(QStringLiteral("notes"));

    EXPECT_EQ(thread->app(), QString());
    EXPECT_TRUE(store->messages(QStringLiteral("notes")).isEmpty());
}

TEST_F(ChatThreadTest, ForgettingAnotherAppLeavesTheCurrentOneAlone)
{
    thread->bind(QStringLiteral("notes"));
    store->append(QStringLiteral("timer"), QStringLiteral("user"), QStringLiteral("t"));

    thread->forget(QStringLiteral("timer"));

    EXPECT_EQ(thread->app(), QStringLiteral("notes"));
    EXPECT_TRUE(store->messages(QStringLiteral("timer")).isEmpty());
}

TEST_F(ChatThreadTest, ResetStartsAFreshScratchThread)
{
    talk(QStringLiteral("hi"), QStringLiteral("hello"));
    thread->bind(QStringLiteral("notes"));

    thread->reset();

    EXPECT_EQ(thread->app(), QString());
    EXPECT_TRUE(controller.messages().isEmpty());
    EXPECT_TRUE(store->messages(QString()).isEmpty());
    EXPECT_EQ(store->messages(QStringLiteral("notes")).size(), 2);
}

TEST_F(ChatThreadTest, AFailedRequestStoresNoReply)
{
    client.script = { Completion {} };
    thread->remember(QStringLiteral("hi"));
    requestAndWait(controller, client, QStringLiteral("hi"));

    EXPECT_EQ(texts(store->messages(QString())), QStringList { QStringLiteral("user:hi") });
}
