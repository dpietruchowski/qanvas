#include <gtest/gtest.h>

#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "runtime/app/app.h"
#include "runtime/app/appschema.h"

class AppSchemaTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit());
        ASSERT_TRUE(QDir().mkpath(app.dir()));
    }

    void TearDown() override
    {
        qunsetenv("QANVAS_APPS_ROOT");
        if (QSqlDatabase::contains(QStringLiteral("schema-test")))
            QSqlDatabase::removeDatabase(QStringLiteral("schema-test"));
    }

    void prepare(const QStringList& statements)
    {
        QSqlDatabase db
            = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("schema-test"));
        db.setDatabaseName(app.databasePath());
        ASSERT_TRUE(db.open());
        for (const QString& statement : statements)
            ASSERT_TRUE(QSqlQuery(db).exec(statement)) << statement.toStdString();
        db.close();
    }

    QTemporaryDir root;
    App app { QStringLiteral("notes") };
};

TEST_F(AppSchemaTest, SaysSoWhenThereIsNoDatabase)
{
    EXPECT_EQ(AppSchema(app).describe(), QStringLiteral("No database yet"));
}

TEST_F(AppSchemaTest, SaysSoWhenTheDatabaseHasNoTables)
{
    prepare({ QStringLiteral("PRAGMA user_version = 1") });

    EXPECT_EQ(AppSchema(app).describe(), QStringLiteral("The database has no tables yet"));
}

TEST_F(AppSchemaTest, ListsTablesWithTheirRowCountsThenIndexes)
{
    prepare({ QStringLiteral("CREATE TABLE items (id INTEGER PRIMARY KEY, text TEXT NOT NULL)"),
              QStringLiteral("CREATE TABLE tags (name TEXT PRIMARY KEY)"),
              QStringLiteral("CREATE INDEX items_text ON items (text)"),
              QStringLiteral("INSERT INTO items (text) VALUES ('milk'), ('bread')"),
              QStringLiteral("INSERT INTO tags VALUES ('food')") });

    const QString described = AppSchema(app).describe();

    EXPECT_EQ(described,
              QStringLiteral("items — 2 rows\n"
                             "CREATE TABLE items (id INTEGER PRIMARY KEY, text TEXT NOT NULL)\n"
                             "\n"
                             "tags — 1 row\n"
                             "CREATE TABLE tags (name TEXT PRIMARY KEY)\n"
                             "\n"
                             "CREATE INDEX items_text ON items (text)"));
}

TEST_F(AppSchemaTest, LeavesTheDatabaseUntouched)
{
    prepare({ QStringLiteral("CREATE TABLE items (id INTEGER PRIMARY KEY)") });

    {
        AppSchema schema(app);
        schema.describe();
    }

    EXPECT_FALSE(QSqlDatabase::contains(QStringLiteral("qanvas_schema_1")));
    EXPECT_TRUE(AppSchema(app).describe().startsWith(QStringLiteral("items — 0 rows")));
}
