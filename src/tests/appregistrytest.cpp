#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QTemporaryDir>
#include <QVariantMap>

#include <QSqlQuery>

#include "runtime/app/appregistry.h"
#include "runtime/storage/systemdatabase.h"

namespace
{
const QString kIcon = QStringLiteral("cart");
const QString kOtherIcon = QStringLiteral("book");

QString iconOf(const AppRegistry& registry, const QString& id)
{
    const QVariantList apps = registry.apps();
    for (const QVariant& app : apps)
    {
        if (app.toMap().value(QStringLiteral("id")).toString() == id)
            return app.toMap().value(QStringLiteral("icon")).toString();
    }
    return QString();
}
}

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

    EXPECT_TRUE(registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon));

    ASSERT_EQ(registry.apps().size(), 1);
    const QVariantMap app = registry.apps().first().toMap();
    EXPECT_EQ(app.value(QStringLiteral("id")).toString(), QStringLiteral("notes"));
    EXPECT_EQ(app.value(QStringLiteral("title")).toString(), QStringLiteral("Notatki"));
    EXPECT_FALSE(app.value(QStringLiteral("updatedAt")).toString().isEmpty());
    EXPECT_EQ(changes, 1);
}

TEST_F(AppRegistryTest, KeepsTheIconItWasRecordedWith)
{
    AppRegistry registry(nullptr);

    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    registry.record(QStringLiteral("timer"), QStringLiteral("Minutnik"), kOtherIcon);

    EXPECT_EQ(iconOf(registry, QStringLiteral("notes")), kIcon);
    EXPECT_EQ(iconOf(registry, QStringLiteral("timer")), kOtherIcon);
}

TEST_F(AppRegistryTest, RecordingTheSameIdAgainReplacesTheIcon)
{
    AppRegistry registry(nullptr);

    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kOtherIcon);

    EXPECT_EQ(iconOf(registry, QStringLiteral("notes")), kOtherIcon);
}

TEST_F(AppRegistryTest, SetIconChangesOnlyThatAppAndAnnouncesIt)
{
    AppRegistry registry(nullptr);
    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    registry.record(QStringLiteral("timer"), QStringLiteral("Minutnik"), kIcon);
    int changes = 0;
    QObject::connect(&registry, &AppRegistry::appsChanged, [&changes] { ++changes; });

    EXPECT_TRUE(registry.setIcon(QStringLiteral("timer"), kOtherIcon));

    EXPECT_EQ(iconOf(registry, QStringLiteral("timer")), kOtherIcon);
    EXPECT_EQ(iconOf(registry, QStringLiteral("notes")), kIcon);
    EXPECT_EQ(registry.apps().first().toMap().value(QStringLiteral("id")).toString(),
              QStringLiteral("timer"));
    EXPECT_EQ(changes, 1);
}

TEST_F(AppRegistryTest, SetIconRefusesAnAppThatIsNotRecordedOrNotASlug)
{
    AppRegistry registry(nullptr);
    int changes = 0;
    QObject::connect(&registry, &AppRegistry::appsChanged, [&changes] { ++changes; });

    EXPECT_FALSE(registry.setIcon(QStringLiteral("notes"), kIcon));
    EXPECT_FALSE(registry.setIcon(QStringLiteral("../notes"), kIcon));

    EXPECT_TRUE(registry.apps().isEmpty());
    EXPECT_EQ(changes, 0);
}

TEST_F(AppRegistryTest, AnAppRecordedBeforeIconsExistedGetsTheDefaultIcon)
{
    {
        SystemDatabase legacy(QStringLiteral("legacy"));
        ASSERT_TRUE(legacy.isOpen());
        QSqlQuery q(legacy.database());
        ASSERT_TRUE(q.exec(QStringLiteral("CREATE TABLE apps (id TEXT PRIMARY KEY, title TEXT NOT "
                                          "NULL, created_at TEXT NOT NULL, updated_at TEXT NOT "
                                          "NULL)")));
        ASSERT_TRUE(q.exec(QStringLiteral("INSERT INTO apps VALUES ('notes', 'Notatki', "
                                          "'2026-09-01T00:00:00Z', '2026-09-01T00:00:00Z')")));
    }

    AppRegistry registry(nullptr);

    ASSERT_EQ(registry.apps().size(), 1);
    EXPECT_EQ(iconOf(registry, QStringLiteral("notes")), AppRegistry::defaultIcon());
    EXPECT_TRUE(registry.setIcon(QStringLiteral("notes"), kIcon));
    EXPECT_EQ(iconOf(registry, QStringLiteral("notes")), kIcon);
}

TEST_F(AppRegistryTest, ReopeningARegistryThatHasTheIconColumnKeepsTheIcons)
{
    {
        AppRegistry registry(nullptr);
        registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    }

    AppRegistry reopened(nullptr);

    EXPECT_EQ(iconOf(reopened, QStringLiteral("notes")), kIcon);
}

TEST_F(AppRegistryTest, RecordingTheSameIdKeepsOneRowAndUpdatesTheTitle)
{
    AppRegistry registry(nullptr);

    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    registry.record(QStringLiteral("notes"), QStringLiteral("Notatnik"), kIcon);

    ASSERT_EQ(registry.apps().size(), 1);
    EXPECT_EQ(registry.apps().first().toMap().value(QStringLiteral("title")).toString(),
              QStringLiteral("Notatnik"));
}

TEST_F(AppRegistryTest, ListsTheMostRecentlyUpdatedFirst)
{
    AppRegistry registry(nullptr);

    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    registry.record(QStringLiteral("timer"), QStringLiteral("Minutnik"), kIcon);
    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);

    const QVariantList apps = registry.apps();
    ASSERT_EQ(apps.size(), 2);
    EXPECT_EQ(apps.first().toMap().value(QStringLiteral("id")).toString(), QStringLiteral("notes"));
}

TEST_F(AppRegistryTest, FallsBackToTheIdWhenTheTitleIsBlank)
{
    AppRegistry registry(nullptr);

    registry.record(QStringLiteral("notes"), QStringLiteral("  "), kIcon);

    EXPECT_EQ(registry.apps().first().toMap().value(QStringLiteral("title")).toString(),
              QStringLiteral("notes"));
}

TEST_F(AppRegistryTest, RejectsAnIdThatIsNotASlug)
{
    AppRegistry registry(nullptr);

    EXPECT_FALSE(registry.record(QStringLiteral("../escape"), QStringLiteral("Escape"), kIcon));
    EXPECT_TRUE(registry.apps().isEmpty());
}

TEST_F(AppRegistryTest, RemoveDeletesTheDirectoryAndTheRow)
{
    AppRegistry registry(nullptr);
    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    registry.record(QStringLiteral("timer"), QStringLiteral("Minutnik"), kIcon);
    const QString dir = root.path() + QStringLiteral("/notes");
    ASSERT_TRUE(QDir().mkpath(dir));
    QFile file(dir + QStringLiteral("/app.db"));
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.close();
    int changes = 0;
    QObject::connect(&registry, &AppRegistry::appsChanged, [&changes] { ++changes; });

    EXPECT_TRUE(registry.remove(QStringLiteral("notes")));

    EXPECT_FALSE(QDir(dir).exists());
    ASSERT_EQ(registry.apps().size(), 1);
    EXPECT_EQ(registry.apps().first().toMap().value(QStringLiteral("id")).toString(),
              QStringLiteral("timer"));
    EXPECT_EQ(changes, 1);
}

TEST_F(AppRegistryTest, RemoveAnnouncesTheAppWhileItsFilesAreStillThere)
{
    AppRegistry registry(nullptr);
    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    const QString dir = root.path() + QStringLiteral("/notes");
    ASSERT_TRUE(QDir().mkpath(dir));
    QStringList announced;
    bool filesWereThere = false;
    QObject::connect(&registry, &AppRegistry::appRemoving,
                     [&](const QString& id)
                     {
                         announced.append(id);
                         filesWereThere = QDir(dir).exists();
                     });

    registry.remove(QStringLiteral("notes"));

    EXPECT_EQ(announced, QStringList { QStringLiteral("notes") });
    EXPECT_TRUE(filesWereThere);
}

TEST_F(AppRegistryTest, GrantsACapabilityOnceAndListsIt)
{
    AppRegistry registry(nullptr);

    EXPECT_FALSE(registry.has(QStringLiteral("notes"), QStringLiteral("notes")));
    EXPECT_TRUE(registry.grant(QStringLiteral("notes"), QStringLiteral("notes")));
    EXPECT_TRUE(registry.grant(QStringLiteral("notes"), QStringLiteral("notes")));

    EXPECT_TRUE(registry.has(QStringLiteral("notes"), QStringLiteral("notes")));
    EXPECT_FALSE(registry.has(QStringLiteral("timer"), QStringLiteral("notes")));
    EXPECT_EQ(registry.capabilities(QStringLiteral("notes")),
              QStringList { QStringLiteral("notes") });
}

TEST_F(AppRegistryTest, GrantRejectsNamesThatAreNotSlugs)
{
    AppRegistry registry(nullptr);

    EXPECT_FALSE(registry.grant(QStringLiteral("../notes"), QStringLiteral("notes")));
    EXPECT_FALSE(registry.grant(QStringLiteral("notes"), QStringLiteral("")));
    EXPECT_TRUE(registry.capabilities(QStringLiteral("notes")).isEmpty());
}

TEST_F(AppRegistryTest, RemoveDropsTheCapabilities)
{
    AppRegistry registry(nullptr);
    registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    registry.grant(QStringLiteral("notes"), QStringLiteral("notes"));

    registry.remove(QStringLiteral("notes"));

    EXPECT_FALSE(registry.has(QStringLiteral("notes"), QStringLiteral("notes")));
}

TEST_F(AppRegistryTest, RemoveRejectsAnIdThatIsNotASlug)
{
    AppRegistry registry(nullptr);
    ASSERT_TRUE(QDir().mkpath(root.path() + QStringLiteral("/keep")));

    EXPECT_FALSE(registry.remove(QStringLiteral("../keep")));
    EXPECT_FALSE(registry.remove(QStringLiteral("")));
    EXPECT_TRUE(QDir(root.path() + QStringLiteral("/keep")).exists());
}

TEST_F(AppRegistryTest, SurvivesAcrossInstances)
{
    {
        AppRegistry registry(nullptr);
        registry.record(QStringLiteral("notes"), QStringLiteral("Notatki"), kIcon);
    }

    AppRegistry reopened(nullptr);

    ASSERT_EQ(reopened.apps().size(), 1);
    EXPECT_EQ(reopened.apps().first().toMap().value(QStringLiteral("id")).toString(),
              QStringLiteral("notes"));
}
