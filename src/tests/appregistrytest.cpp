#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>
#include <QVariantMap>

#include "runtime/appregistry.h"

class AppRegistryTest : public ::testing::Test
{
protected:
    void SetUp() override { qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit()); }
    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
};

TEST_F(AppRegistryTest, KeepsItsDatabaseInTheAppsRoot)
{
    AppRegistry registry(nullptr);

    EXPECT_TRUE(QFile::exists(root.path() + QStringLiteral("/system.db")));
}

TEST_F(AppRegistryTest, RecordsAnAppAndListsIt)
{
    AppRegistry registry(nullptr);
    int changes = 0;
    QObject::connect(&registry, &AppRegistry::appsChanged, [&changes] { ++changes; });

    EXPECT_TRUE(registry.record(QStringLiteral("notes"), QStringLiteral("Notatki")));

    ASSERT_EQ(registry.apps().size(), 1);
    const QVariantMap app = registry.apps().first().toMap();
    EXPECT_EQ(app.value(QStringLiteral("id")).toString(), QStringLiteral("notes"));
    EXPECT_EQ(app.value(QStringLiteral("title")).toString(), QStringLiteral("Notatki"));
    EXPECT_FALSE(app.value(QStringLiteral("updatedAt")).toString().isEmpty());
    EXPECT_EQ(changes, 1);
}

TEST_F(AppRegistryTest, RecordingTheSameIdKeepsOneRowAndUpdatesTheTitle)
{
    AppRegistry registry(nullptr);

    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"));
    registry.record(QStringLiteral("notes"), QStringLiteral("Notatnik"));

    ASSERT_EQ(registry.apps().size(), 1);
    EXPECT_EQ(registry.apps().first().toMap().value(QStringLiteral("title")).toString(),
              QStringLiteral("Notatnik"));
}

TEST_F(AppRegistryTest, ListsTheMostRecentlyUpdatedFirst)
{
    AppRegistry registry(nullptr);

    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"));
    registry.record(QStringLiteral("timer"), QStringLiteral("Minutnik"));
    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"));

    const QVariantList apps = registry.apps();
    ASSERT_EQ(apps.size(), 2);
    EXPECT_EQ(apps.first().toMap().value(QStringLiteral("id")).toString(), QStringLiteral("notes"));
}

TEST_F(AppRegistryTest, FallsBackToTheIdWhenTheTitleIsBlank)
{
    AppRegistry registry(nullptr);

    registry.record(QStringLiteral("notes"), QStringLiteral("  "));

    EXPECT_EQ(registry.apps().first().toMap().value(QStringLiteral("title")).toString(),
              QStringLiteral("notes"));
}

TEST_F(AppRegistryTest, RejectsAnIdThatIsNotASlug)
{
    AppRegistry registry(nullptr);

    EXPECT_FALSE(registry.record(QStringLiteral("../escape"), QStringLiteral("Escape")));
    EXPECT_TRUE(registry.apps().isEmpty());
}

TEST_F(AppRegistryTest, SurvivesAcrossInstances)
{
    {
        AppRegistry registry(nullptr);
        registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"));
    }

    AppRegistry reopened(nullptr);

    ASSERT_EQ(reopened.apps().size(), 1);
    EXPECT_EQ(reopened.apps().first().toMap().value(QStringLiteral("id")).toString(),
              QStringLiteral("notes"));
}
