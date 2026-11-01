#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <QDir>
#include <QFile>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include "async/backendworker.h"
#include "isolatedsettings.h"
#include "libs/tests/chat/chatfixture.h"
#include "libs/tests/chat/fakeclient.h"
#include "runtime/app/app.h"
#include "runtime/app/appregistry.h"
#include "runtime/chat/chathost.h"
#include "runtime/chat/systemprompt.h"

using namespace chat_test;

namespace
{
constexpr int kWaitMilliseconds = 5000;
const QString kMainQml = QStringLiteral("import QtQuick\nItem {}\n");
const QString kTask = QStringLiteral("Make a notes app.");

QVariantMap createApp(const QString& id)
{
    return QVariantMap { { QStringLiteral("id"), id },
                         { QStringLiteral("title"), id },
                         { QStringLiteral("qml"), kMainQml } };
}

QVariantMap writeUnusedScreen(const QString& id)
{
    return QVariantMap { { QStringLiteral("app"), id },
                         { QStringLiteral("path"), QStringLiteral("Card.qml") },
                         { QStringLiteral("content"), kMainQml } };
}

QString lastMessageOf(const ClientState& client)
{
    return client.held.front().messages.constLast().content;
}
}

class ChatHostTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit());
        registry = std::make_unique<AppRegistry>(nullptr);
        engine.setOutputWarningsToStandardError(false);
    }

    void TearDown() override
    {
        host.reset();
        registry.reset();
        qunsetenv("QANVAS_APPS_ROOT");
    }

    chat::ChatModule::Clients clients()
    {
        return chat::ChatModule::Clients {
            .factory = [this](const ClaudeSession& session) { return makeClient(session); },
            .available = [] { return true; },
            .forget = [](const QString&) {},
            .loginCommand = QString(),
        };
    }

    std::unique_ptr<::Client> makeClient(const ClaudeSession& session)
    {
        auto state = std::make_shared<ClientState>();
        state->requested = session;
        state->oneShot = true;
        m_clients.push_back(state);
        return std::make_unique<ScriptedClient>(state);
    }

    ChatHost::Config config(const QString& directory) const
    {
        return ChatHost::Config { .dataDirectory = directory,
                                  .settingsOrganization = IsolatedSettings::organization(),
                                  .settingsApplication = IsolatedSettings::application() };
    }

    void start()
    {
        host
            = std::make_unique<ChatHost>(config(data.path()), clients(), engine, worker, *registry);
        ASSERT_TRUE(host->isReady());
    }

    void restart()
    {
        host.reset();
        start();
    }

    void createFinishedApp(const QString& id)
    {
        registry->record(id, id);
        QDir().mkpath(App(id).dir());
        QFile entry(App(id).entryPath());
        ASSERT_TRUE(entry.open(QIODevice::WriteOnly));
        entry.write(kMainQml.toUtf8());
    }

    QUuid startConversation(const QString& text = QStringLiteral("question"))
    {
        const QUuid id = awaitOk(host->chat()->store().createConversation(text)).id();
        EXPECT_TRUE(host->chat()->runner().send(id, text));
        return id;
    }

    ClientState& client(size_t index) { return *m_clients.at(index); }
    ClientState& newest() { return *m_clients.back(); }

    bool waitForRequest(size_t index)
    {
        return QTest::qWaitFor(
            [this, index] { return m_clients.size() > index && !m_clients[index]->held.empty(); },
            kWaitMilliseconds);
    }

    bool waitUntilIdle()
    {
        return QTest::qWaitFor([this] { return !host->busy(); }, kWaitMilliseconds);
    }

    void delegateToTheBuilder(ClientState& main, const QString& task = kTask)
    {
        main.completeNext(toolCallCompletion(QStringLiteral("call-builder"),
                                             QStringLiteral("builder"),
                                             { { QStringLiteral("task"), task } }));
    }

    void answer(ClientState& state, const QString& text)
    {
        state.completeNext(contentCompletion(text));
    }

    QTemporaryDir root;
    QTemporaryDir data;
    IsolatedSettings isolated;
    QQmlEngine engine;
    BackendWorker worker;
    std::unique_ptr<AppRegistry> registry;
    std::unique_ptr<ChatHost> host;
    std::vector<std::shared_ptr<ClientState>> m_clients;
};

TEST_F(ChatHostTest, IsReadyAndHasNoProviderSettingsWhenTheClientsAreGiven)
{
    start();

    EXPECT_NE(host->chat(), nullptr);
    EXPECT_EQ(host->providerSettings(), nullptr);
}

TEST_F(ChatHostTest, IsNotReadyWhenTheStorageCannotBeOpened)
{
    QFile blocker(data.filePath(QStringLiteral("blocker")));
    ASSERT_TRUE(blocker.open(QIODevice::WriteOnly));

    host = std::make_unique<ChatHost>(config(data.filePath(QStringLiteral("blocker/data"))),
                                      clients(), engine, worker, *registry);

    EXPECT_FALSE(host->isReady());
    EXPECT_EQ(host->chat(), nullptr);
    EXPECT_FALSE(host->busy());
}

TEST_F(ChatHostTest, IsIdleWhenNothingRuns)
{
    start();

    EXPECT_FALSE(host->busy());
}

TEST_F(ChatHostTest, IsBusyUntilTheLastRunningConversationEnds)
{
    start();
    QSignalSpy busyChanged(host.get(), &ChatHost::busyChanged);
    const QUuid first = startConversation(QStringLiteral("first"));
    ASSERT_TRUE(waitForRequest(0));
    const QUuid second = startConversation(QStringLiteral("second"));
    ASSERT_TRUE(waitForRequest(1));

    EXPECT_TRUE(host->busy());
    EXPECT_EQ(busyChanged.count(), 1);

    answer(client(0), QStringLiteral("one"));
    ASSERT_TRUE(
        QTest::qWaitFor([&] { return !host->chat()->runner().isBusy(first); }, kWaitMilliseconds));

    EXPECT_TRUE(host->busy());
    EXPECT_EQ(busyChanged.count(), 1);

    answer(client(1), QStringLiteral("two"));
    ASSERT_TRUE(waitUntilIdle());

    EXPECT_FALSE(host->chat()->runner().isBusy(second));
    EXPECT_EQ(busyChanged.count(), 2);
}

TEST_F(ChatHostTest, TheMainAgentCanHandATaskToTheBuilder)
{
    start();
    startConversation();
    ASSERT_TRUE(waitForRequest(0));

    delegateToTheBuilder(client(0));

    ASSERT_TRUE(waitForRequest(1));
    const ClientRequest& request = client(1).held.front();
    EXPECT_EQ(request.messages.first().content, SystemPrompt::text());
    EXPECT_TRUE(request.messages.constLast().content.startsWith(kTask));
    EXPECT_TRUE(request.tools.contains(QStringLiteral("create_app")));
    EXPECT_TRUE(request.tools.contains(QStringLiteral("read_log")));
    EXPECT_EQ(request.tools.value(QStringLiteral("create_app")).effect, ToolEffect::Writes);
    EXPECT_EQ(request.tools.value(QStringLiteral("read_log")).effect, ToolEffect::Reads);
}

TEST_F(ChatHostTest, AnAppTheBuilderBuildsIsAnnounced)
{
    start();
    QSignalSpy shown(host.get(), &ChatHost::appShown);
    startConversation();
    ASSERT_TRUE(waitForRequest(0));
    delegateToTheBuilder(client(0));
    ASSERT_TRUE(waitForRequest(1));

    client(1).completeNext(toolCallCompletion(QStringLiteral("call-create"),
                                              QStringLiteral("create_app"),
                                              createApp(QStringLiteral("notes"))));
    ASSERT_TRUE(waitForRequest(1));

    ASSERT_EQ(shown.count(), 1);
    EXPECT_EQ(shown.first().first().toString(), QStringLiteral("notes"));
    EXPECT_TRUE(App(QStringLiteral("notes")).exists());

    answer(client(1), QStringLiteral("Built the notes app."));
    ASSERT_TRUE(waitForRequest(0));
    EXPECT_TRUE(lastMessageOf(client(0)).contains(QStringLiteral("Built the notes app.")));

    answer(client(0), QStringLiteral("Your notes app is ready."));
    ASSERT_TRUE(waitUntilIdle());
}

TEST_F(ChatHostTest, TheBuilderIsSentBackToWhatItLeftUndone)
{
    start();
    startConversation();
    ASSERT_TRUE(waitForRequest(0));
    delegateToTheBuilder(client(0));
    ASSERT_TRUE(waitForRequest(1));
    client(1).completeNext(toolCallCompletion(QStringLiteral("call-create"),
                                              QStringLiteral("create_app"),
                                              createApp(QStringLiteral("notes"))));
    ASSERT_TRUE(waitForRequest(1));
    client(1).completeNext(toolCallCompletion(QStringLiteral("call-write"),
                                              QStringLiteral("write_file"),
                                              writeUnusedScreen(QStringLiteral("notes"))));
    ASSERT_TRUE(waitForRequest(1));

    answer(client(1), QStringLiteral("Done."));

    ASSERT_TRUE(waitForRequest(2));
    const QString text = lastMessageOf(client(2));
    EXPECT_TRUE(text.startsWith(QStringLiteral("Runtime check — the app is not finished:")))
        << qPrintable(text);
    EXPECT_TRUE(text.contains(QStringLiteral("Card.qml"))) << qPrintable(text);
}

TEST_F(ChatHostTest, TheBuilderIsLeftAloneWhenItFinishedEverything)
{
    start();
    startConversation();
    ASSERT_TRUE(waitForRequest(0));
    delegateToTheBuilder(client(0));
    ASSERT_TRUE(waitForRequest(1));
    client(1).completeNext(toolCallCompletion(QStringLiteral("call-create"),
                                              QStringLiteral("create_app"),
                                              createApp(QStringLiteral("notes"))));
    ASSERT_TRUE(waitForRequest(1));

    answer(client(1), QStringLiteral("Done."));

    ASSERT_TRUE(waitForRequest(0));
    EXPECT_EQ(m_clients.size(), 2u);
}

TEST_F(ChatHostTest, ANewTurnDoesNotInheritWhatTheLastTurnLeftUndone)
{
    start();
    startConversation(QStringLiteral("first"));
    ASSERT_TRUE(waitForRequest(0));
    delegateToTheBuilder(client(0));
    ASSERT_TRUE(waitForRequest(1));
    client(1).completeNext(toolCallCompletion(QStringLiteral("call-create"),
                                              QStringLiteral("create_app"),
                                              createApp(QStringLiteral("notes"))));
    ASSERT_TRUE(waitForRequest(1));
    client(1).completeNext(toolCallCompletion(QStringLiteral("call-write"),
                                              QStringLiteral("write_file"),
                                              writeUnusedScreen(QStringLiteral("notes"))));
    ASSERT_TRUE(waitForRequest(1));
    answer(client(1), QStringLiteral("first report"));
    ASSERT_TRUE(waitForRequest(2));
    answer(client(2), QStringLiteral("second report"));
    ASSERT_TRUE(waitForRequest(3));
    answer(client(3), QStringLiteral("third report"));
    ASSERT_TRUE(waitForRequest(0));
    answer(client(0), QStringLiteral("finished"));
    ASSERT_TRUE(waitUntilIdle());
    ASSERT_EQ(m_clients.size(), 4u);

    startConversation(QStringLiteral("second"));
    ASSERT_TRUE(waitForRequest(4));
    delegateToTheBuilder(client(4), QStringLiteral("Look at the notes app."));
    ASSERT_TRUE(waitForRequest(5));
    answer(client(5), QStringLiteral("Nothing to change."));

    ASSERT_TRUE(waitForRequest(4));
    EXPECT_EQ(m_clients.size(), 6u);
    answer(client(4), QStringLiteral("All good."));
    ASSERT_TRUE(waitUntilIdle());
}

TEST_F(ChatHostTest, KnowsNoConversationForAnAppThatWasNeverBound)
{
    start();

    EXPECT_TRUE(host->conversationFor(QStringLiteral("notes")).isEmpty());
    EXPECT_TRUE(host->appFor(QStringLiteral("c-1")).isEmpty());
}

TEST_F(ChatHostTest, RemembersTheConversationOfAnAppAndTheOtherWayRound)
{
    start();

    host->bind(QStringLiteral("notes"), QStringLiteral("c-1"));

    EXPECT_EQ(host->conversationFor(QStringLiteral("notes")), QStringLiteral("c-1"));
    EXPECT_EQ(host->appFor(QStringLiteral("c-1")), QStringLiteral("notes"));
}

TEST_F(ChatHostTest, TheNewestConversationOfAnAppWins)
{
    start();
    host->bind(QStringLiteral("notes"), QStringLiteral("c-1"));

    host->bind(QStringLiteral("notes"), QStringLiteral("c-2"));

    EXPECT_EQ(host->conversationFor(QStringLiteral("notes")), QStringLiteral("c-2"));
    EXPECT_TRUE(host->appFor(QStringLiteral("c-1")).isEmpty());
}

TEST_F(ChatHostTest, TheConversationsOfAnAppSurviveARestart)
{
    start();
    host->bind(QStringLiteral("notes"), QStringLiteral("c-1"));

    restart();

    EXPECT_EQ(host->conversationFor(QStringLiteral("notes")), QStringLiteral("c-1"));
}

TEST_F(ChatHostTest, RemovingAnAppForgetsItsConversationAndNoOtherOne)
{
    start();
    createFinishedApp(QStringLiteral("notes"));
    createFinishedApp(QStringLiteral("timer"));
    host->bind(QStringLiteral("notes"), QStringLiteral("c-1"));
    host->bind(QStringLiteral("timer"), QStringLiteral("c-2"));

    registry->remove(QStringLiteral("notes"));

    EXPECT_TRUE(host->conversationFor(QStringLiteral("notes")).isEmpty());
    EXPECT_TRUE(host->appFor(QStringLiteral("c-1")).isEmpty());
    EXPECT_EQ(host->conversationFor(QStringLiteral("timer")), QStringLiteral("c-2"));
}

TEST_F(ChatHostTest, RemovingAConversationForgetsItsAppAndNoOtherOne)
{
    start();
    const QString removed = awaitOk(host->chat()->store().createConversation(QStringLiteral("one")))
                                .id()
                                .toString(QUuid::WithoutBraces);
    const QString kept = awaitOk(host->chat()->store().createConversation(QStringLiteral("two")))
                             .id()
                             .toString(QUuid::WithoutBraces);
    host->bind(QStringLiteral("notes"), removed);
    host->bind(QStringLiteral("timer"), kept);
    ASSERT_EQ(host->conversationFor(QStringLiteral("notes")), removed);

    host->chat()->remove(removed);

    ASSERT_TRUE(
        QTest::qWaitFor([this] { return host->conversationFor(QStringLiteral("notes")).isEmpty(); },
                        kWaitMilliseconds));
    EXPECT_TRUE(host->appFor(removed).isEmpty());
    EXPECT_EQ(host->conversationFor(QStringLiteral("timer")), kept);
}

TEST_F(ChatHostTest, HasNoLastAppAtFirst)
{
    start();

    EXPECT_TRUE(host->lastAppId().isEmpty());
}

TEST_F(ChatHostTest, RemembersTheLastAppAcrossRestarts)
{
    start();
    createFinishedApp(QStringLiteral("notes"));
    QSignalSpy changed(host.get(), &ChatHost::lastAppIdChanged);

    host->setLastAppId(QStringLiteral("notes"));
    host->setLastAppId(QStringLiteral("notes"));

    EXPECT_EQ(host->lastAppId(), QStringLiteral("notes"));
    EXPECT_EQ(changed.count(), 1);

    restart();

    EXPECT_EQ(host->lastAppId(), QStringLiteral("notes"));
}

TEST_F(ChatHostTest, TheLastAppIsEmptyOnceTheAppIsGone)
{
    start();
    createFinishedApp(QStringLiteral("notes"));
    host->setLastAppId(QStringLiteral("notes"));
    QFile::remove(App(QStringLiteral("notes")).entryPath());

    EXPECT_TRUE(host->lastAppId().isEmpty());

    restart();

    EXPECT_TRUE(host->lastAppId().isEmpty());
}

TEST_F(ChatHostTest, RemovingTheLastAppClearsItAndSaysSo)
{
    start();
    createFinishedApp(QStringLiteral("notes"));
    createFinishedApp(QStringLiteral("timer"));
    host->setLastAppId(QStringLiteral("notes"));
    QSignalSpy changed(host.get(), &ChatHost::lastAppIdChanged);

    registry->remove(QStringLiteral("timer"));
    EXPECT_EQ(changed.count(), 0);
    EXPECT_EQ(host->lastAppId(), QStringLiteral("notes"));

    registry->remove(QStringLiteral("notes"));

    EXPECT_EQ(changed.count(), 1);
    EXPECT_TRUE(host->lastAppId().isEmpty());

    restart();

    EXPECT_TRUE(host->lastAppId().isEmpty());
}
