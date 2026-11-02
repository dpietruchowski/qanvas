#include <gtest/gtest.h>

#include <memory>

#include <QByteArray>
#include <QMap>
#include <QSignalSpy>
#include <QStringList>
#include <QTemporaryDir>
#include <QUuid>
#include <QVariantMap>

#include "agent/agent.h"
#include "agent/anthropicclient.h"
#include "agent/claudesession.h"
#include "async/backendworker.h"
#include "chat/application/agentcatalog.h"
#include "chat/chatmodule.h"
#include "chat/ui/chat.h"
#include "isolatedsettings.h"
#include "platform/securestorage.h"
#include "runtime/chat/providersettings.h"

namespace
{
const QStringList kKeyVariables { QStringLiteral("OPENAI_API_KEY"),
                                  QStringLiteral("CLAUDE_API_KEY"),
                                  QStringLiteral("OPENROUTER_API_KEY") };
const QString kLongKey = QStringLiteral("sk-abcdefghijklmnop1234");
}

class ProviderSettingsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        for (const QString& name : kKeyVariables)
        {
            const QByteArray variable = name.toLatin1();
            if (qEnvironmentVariableIsSet(variable.constData()))
                saved.insert(name, qgetenv(variable.constData()));
            qunsetenv(variable.constData());
        }
        service
            = QStringLiteral("qanvas-test-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    void TearDown() override
    {
        module.reset();
        forgetStoredKeys();
        for (const QString& name : kKeyVariables)
        {
            const QByteArray variable = name.toLatin1();
            if (saved.contains(name))
                qputenv(variable.constData(), saved.value(name));
            else
                qunsetenv(variable.constData());
        }
    }

    std::unique_ptr<ProviderSettings> make() { return std::make_unique<ProviderSettings>(service); }

    void forgetStoredKeys()
    {
        ProviderSettings settings(service);
        for (const QString& provider : settings.providers())
        {
            settings.setProvider(provider);
            settings.setApiKey(QString());
        }
    }

    void bindToChat(ProviderSettings& settings)
    {
        module = chat::ChatModule::create(
            chat::ChatModule::Config {
                .databasePath = directory.filePath(QStringLiteral("chat.db")),
                .workingDirectory = directory.filePath(QStringLiteral("work")),
                .settingsOrganization = IsolatedSettings::organization(),
                .settingsApplication = IsolatedSettings::application(),
                .settingsGroup = QStringLiteral("chat"),
                .worker = &worker,
                .appName = QStringLiteral("Test"),
                .appPurpose = QStringLiteral("Tests things."),
            },
            settings.clients());
        ASSERT_NE(module, nullptr);
        ASSERT_TRUE(module->addAgent(
            chat::AppAgent { QStringLiteral("courses"), QStringLiteral("Courses"),
                             QStringLiteral("Handles courses."),
                             ::Agent(QStringLiteral("haiku"), QStringLiteral("prompt")) }));
        ASSERT_TRUE(module->addAgent(chat::AppAgent {
            QStringLiteral("builder"), QStringLiteral("Builder"), QStringLiteral("Builds apps."),
            ::Agent(QStringLiteral("haiku"), QStringLiteral("prompt")) }));
        settings.bind(module->chat());
    }

    chat::Chat& chat() { return module->chat(); }

    QString chosenModelOf(const QString& key)
    {
        for (const QVariant& agent : chat().agents())
        {
            const QVariantMap entry = agent.toMap();
            if (entry.value(QStringLiteral("key")).toString() == key)
                return entry.value(QStringLiteral("chosen")).toString();
        }
        return QStringLiteral("<no such agent>");
    }

    QString chosenAgentModel() { return chosenModelOf(QStringLiteral("courses")); }

    IsolatedSettings isolated;
    QTemporaryDir directory;
    BackendWorker worker;
    std::unique_ptr<chat::ChatModule> module;
    QMap<QString, QByteArray> saved;
    QString service;
};

TEST_F(ProviderSettingsTest, OffersOpenAiOpenRouterAndClaude)
{
    EXPECT_EQ(make()->providers(),
              (QStringList { QStringLiteral("OpenAI"), QStringLiteral("OpenRouter"),
                             QStringLiteral("Claude") }));
}

TEST_F(ProviderSettingsTest, StartsWithOpenAi)
{
    EXPECT_EQ(make()->provider(), QStringLiteral("OpenAI"));
}

TEST_F(ProviderSettingsTest, RemembersTheProviderBetweenRuns)
{
    make()->setProvider(QStringLiteral("Claude"));

    EXPECT_EQ(make()->provider(), QStringLiteral("Claude"));
}

TEST_F(ProviderSettingsTest, ChangingTheProviderIsAnnouncedOnce)
{
    const auto settings = make();
    QSignalSpy changed(settings.get(), &ProviderSettings::providerChanged);

    settings->setProvider(QStringLiteral("OpenRouter"));
    settings->setProvider(QStringLiteral("OpenRouter"));

    EXPECT_EQ(changed.count(), 1);
}

TEST_F(ProviderSettingsTest, WithoutAnyKeyThereIsNoClient)
{
    EXPECT_EQ(make()->createClient(), nullptr);
}

TEST_F(ProviderSettingsTest, AKeyFromTheEnvironmentGivesAClient)
{
    qputenv("OPENAI_API_KEY", "sk-from-environment");

    EXPECT_NE(make()->createClient(), nullptr);
}

TEST_F(ProviderSettingsTest, TheEnvironmentKeyOfAnotherProviderGivesNoClient)
{
    qputenv("CLAUDE_API_KEY", "sk-from-environment");

    EXPECT_EQ(make()->createClient(), nullptr);
}

TEST_F(ProviderSettingsTest, AKeyTypedInGivesAClientAndAPreviewOfItself)
{
    const auto settings = make();

    settings->setApiKey(kLongKey);

    EXPECT_NE(settings->createClient(), nullptr);
    EXPECT_EQ(settings->apiKey(), kLongKey);
    EXPECT_EQ(settings->apiKeyPreview(), QStringLiteral("sk-abcde…1234"));
}

TEST_F(ProviderSettingsTest, AShortKeyIsNotShownInThePreview)
{
    const auto settings = make();

    settings->setApiKey(QStringLiteral("short"));

    EXPECT_EQ(settings->apiKeyPreview(), QStringLiteral("…"));
}

TEST_F(ProviderSettingsTest, WithoutAKeyThePreviewIsEmpty)
{
    EXPECT_TRUE(make()->apiKeyPreview().isEmpty());
}

TEST_F(ProviderSettingsTest, ClearingTheKeyRemovesTheClient)
{
    const auto settings = make();
    settings->setApiKey(kLongKey);

    settings->setApiKey(QString());

    EXPECT_EQ(settings->createClient(), nullptr);
    EXPECT_TRUE(settings->apiKeyPreview().isEmpty());
}

TEST_F(ProviderSettingsTest, ChangingTheKeyIsAnnouncedWithItsHint)
{
    const auto settings = make();
    QSignalSpy keyChanged(settings.get(), &ProviderSettings::apiKeyChanged);
    QSignalSpy hintChanged(settings.get(), &ProviderSettings::apiKeyHintChanged);

    settings->setApiKey(kLongKey);

    EXPECT_EQ(keyChanged.count(), 1);
    EXPECT_EQ(hintChanged.count(), 1);
}

TEST_F(ProviderSettingsTest, TheHintNamesTheEnvironmentVariableThatHoldsTheKey)
{
    qputenv("OPENAI_API_KEY", "sk-from-environment");

    EXPECT_EQ(make()->apiKeyHint(), QStringLiteral("OPENAI_API_KEY"));
}

TEST_F(ProviderSettingsTest, TheHintAsksForAKeyWhenNothingProvidesOne)
{
    const auto settings = make();
    settings->setProvider(QStringLiteral("Claude"));
    const bool keyring = SecureStorage(QCoreApplication::organizationName(), service).isAvailable();

    EXPECT_EQ(settings->apiKeyHint(),
              keyring ? QStringLiteral("Paste your key")
                      : QStringLiteral("No keyring — set CLAUDE_API_KEY"));
}

TEST_F(ProviderSettingsTest, TheHintIsEmptyOnceAKeyIsTypedIn)
{
    const auto settings = make();

    settings->setApiKey(kLongKey);

    EXPECT_TRUE(settings->apiKeyHint().isEmpty());
}

TEST_F(ProviderSettingsTest, TheClientsAreAlwaysAvailableAndNeedNoLogin)
{
    const auto settings = make();
    const chat::ChatModule::Clients clients = settings->clients();

    EXPECT_TRUE(clients.available());
    EXPECT_TRUE(clients.loginCommand.isEmpty());
    clients.forget(QStringLiteral("session"));
}

TEST_F(ProviderSettingsTest, TheClientFactoryFollowsTheKey)
{
    const auto settings = make();
    const chat::ChatModule::Clients clients = settings->clients();

    EXPECT_EQ(clients.factory(ClaudeSession {}), nullptr);

    qputenv("OPENAI_API_KEY", "sk-from-environment");

    EXPECT_NE(clients.factory(ClaudeSession {}), nullptr);
}

TEST_F(ProviderSettingsTest, ClaudeGetsTheNativeClientAndTheOtherProvidersTheCompatibleOne)
{
    const auto settings = make();
    const chat::ChatModule::Clients clients = settings->clients();
    qputenv("OPENAI_API_KEY", "sk-openai-from-environment");
    qputenv("OPENROUTER_API_KEY", "sk-openrouter-from-environment");
    qputenv("CLAUDE_API_KEY", "sk-claude-from-environment");

    const auto openAi = clients.factory(ClaudeSession {});
    settings->setProvider(QStringLiteral("OpenRouter"));
    const auto openRouter = clients.factory(ClaudeSession {});
    settings->setProvider(QStringLiteral("Claude"));
    const auto claude = clients.factory(ClaudeSession {});

    ASSERT_NE(openAi, nullptr);
    ASSERT_NE(openRouter, nullptr);
    ASSERT_NE(claude, nullptr);
    EXPECT_EQ(dynamic_cast<AnthropicClient*>(openAi.get()), nullptr);
    EXPECT_EQ(dynamic_cast<AnthropicClient*>(openRouter.get()), nullptr);
    EXPECT_NE(dynamic_cast<AnthropicClient*>(claude.get()), nullptr);
}

TEST_F(ProviderSettingsTest, BindingGivesTheChatTheDefaultsOfTheProvider)
{
    const auto settings = make();

    bindToChat(*settings);

    EXPECT_EQ(chat().model(), QStringLiteral("gpt-5-mini"));
    EXPECT_EQ(chat().reasoningEffort(), QStringLiteral("Minimal"));
}

TEST_F(ProviderSettingsTest, SwitchingTheProviderSwitchesTheModel)
{
    const auto settings = make();
    bindToChat(*settings);

    settings->setProvider(QStringLiteral("Claude"));
    EXPECT_EQ(chat().model(), QStringLiteral("claude-sonnet-5-5"));

    settings->setProvider(QStringLiteral("OpenRouter"));
    EXPECT_EQ(chat().model(), QStringLiteral("openai/gpt-5-mini"));
}

TEST_F(ProviderSettingsTest, TheModelAndEffortChosenForAProviderComeBackWithIt)
{
    const auto settings = make();
    bindToChat(*settings);
    chat().setModel(QStringLiteral("gpt-5"));
    chat().setReasoningEffort(QStringLiteral("High"));

    settings->setProvider(QStringLiteral("Claude"));
    EXPECT_EQ(chat().model(), QStringLiteral("claude-sonnet-5-5"));
    EXPECT_EQ(chat().reasoningEffort(), QStringLiteral("Minimal"));

    settings->setProvider(QStringLiteral("OpenAI"));
    EXPECT_EQ(chat().model(), QStringLiteral("gpt-5"));
    EXPECT_EQ(chat().reasoningEffort(), QStringLiteral("High"));
}

TEST_F(ProviderSettingsTest, TheModelChosenForAProviderSurvivesARestart)
{
    {
        const auto settings = make();
        bindToChat(*settings);
        chat().setModel(QStringLiteral("gpt-5"));
        module.reset();
    }

    const auto settings = make();
    bindToChat(*settings);

    EXPECT_EQ(chat().model(), QStringLiteral("gpt-5"));
}

TEST_F(ProviderSettingsTest, BindingStartsEveryAgentWithoutAChosenModel)
{
    const auto settings = make();

    bindToChat(*settings);

    EXPECT_TRUE(chosenAgentModel().isEmpty());
}

TEST_F(ProviderSettingsTest, TheModelChosenForAnAgentComesBackWithItsProvider)
{
    const auto settings = make();
    bindToChat(*settings);
    chat().setAgentModel(QStringLiteral("courses"), QStringLiteral("gpt-5"));

    settings->setProvider(QStringLiteral("Claude"));
    EXPECT_TRUE(chosenAgentModel().isEmpty());
    chat().setAgentModel(QStringLiteral("courses"), QStringLiteral("claude-opus-4-1"));

    settings->setProvider(QStringLiteral("OpenAI"));
    EXPECT_EQ(chosenAgentModel(), QStringLiteral("gpt-5"));
    settings->setProvider(QStringLiteral("Claude"));
    EXPECT_EQ(chosenAgentModel(), QStringLiteral("claude-opus-4-1"));
}

TEST_F(ProviderSettingsTest, SwitchingTheProviderKeepsTheAgentModelOfTheOldOne)
{
    const auto settings = make();
    bindToChat(*settings);
    chat().setAgentModel(QStringLiteral("courses"), QStringLiteral("gpt-5"));

    settings->setProvider(QStringLiteral("OpenRouter"));
    settings->setProvider(QStringLiteral("Claude"));
    settings->setProvider(QStringLiteral("OpenAI"));

    EXPECT_EQ(chosenAgentModel(), QStringLiteral("gpt-5"));
}

TEST_F(ProviderSettingsTest, ClearingTheModelOfAnAgentIsRememberedForItsProvider)
{
    const auto settings = make();
    bindToChat(*settings);
    chat().setAgentModel(QStringLiteral("courses"), QStringLiteral("gpt-5"));
    chat().setAgentModel(QStringLiteral("courses"), QString());

    settings->setProvider(QStringLiteral("Claude"));
    settings->setProvider(QStringLiteral("OpenAI"));

    EXPECT_TRUE(chosenAgentModel().isEmpty());
}

TEST_F(ProviderSettingsTest, TheModelChosenForAnAgentSurvivesARestart)
{
    {
        const auto settings = make();
        bindToChat(*settings);
        chat().setAgentModel(QStringLiteral("courses"), QStringLiteral("gpt-5"));
        module.reset();
    }

    const auto settings = make();
    bindToChat(*settings);

    EXPECT_EQ(chosenAgentModel(), QStringLiteral("gpt-5"));
}

TEST_F(ProviderSettingsTest, ClaudeStartsTheBuilderOnHaikuAndTheOtherAgentsOnTheChatModel)
{
    const auto settings = make();
    bindToChat(*settings);

    settings->setProvider(QStringLiteral("Claude"));

    EXPECT_EQ(chosenModelOf(QStringLiteral("builder")), QStringLiteral("claude-haiku-5-5"));
    EXPECT_TRUE(chosenAgentModel().isEmpty());
}

TEST_F(ProviderSettingsTest, TheOtherProvidersLeaveTheBuilderWithoutAChosenModel)
{
    const auto settings = make();
    bindToChat(*settings);

    EXPECT_TRUE(chosenModelOf(QStringLiteral("builder")).isEmpty());
    settings->setProvider(QStringLiteral("OpenRouter"));
    EXPECT_TRUE(chosenModelOf(QStringLiteral("builder")).isEmpty());
}

TEST_F(ProviderSettingsTest, TheBuilderModelChosenOnClaudeWinsOverTheDefault)
{
    const auto settings = make();
    bindToChat(*settings);
    settings->setProvider(QStringLiteral("Claude"));
    chat().setAgentModel(QStringLiteral("builder"), QStringLiteral("claude-sonnet-5-5"));

    settings->setProvider(QStringLiteral("OpenAI"));
    settings->setProvider(QStringLiteral("Claude"));

    EXPECT_EQ(chosenModelOf(QStringLiteral("builder")), QStringLiteral("claude-sonnet-5-5"));
}

TEST_F(ProviderSettingsTest, ClearingTheBuilderModelOnClaudeIsRememberedOverTheDefault)
{
    const auto settings = make();
    bindToChat(*settings);
    settings->setProvider(QStringLiteral("Claude"));
    chat().setAgentModel(QStringLiteral("builder"), QString());

    settings->setProvider(QStringLiteral("OpenAI"));
    settings->setProvider(QStringLiteral("Claude"));

    EXPECT_TRUE(chosenModelOf(QStringLiteral("builder")).isEmpty());
}

TEST_F(ProviderSettingsTest, TheClaudeDefaultsSurviveARestart)
{
    {
        const auto settings = make();
        bindToChat(*settings);
        settings->setProvider(QStringLiteral("Claude"));
        module.reset();
    }

    const auto settings = make();
    bindToChat(*settings);

    EXPECT_EQ(chat().model(), QStringLiteral("claude-sonnet-5-5"));
    EXPECT_EQ(chosenModelOf(QStringLiteral("builder")), QStringLiteral("claude-haiku-5-5"));
}
