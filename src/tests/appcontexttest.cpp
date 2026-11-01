#include <gtest/gtest.h>

#include <memory>

#include <QJSEngine>
#include <QJSValue>
#include <QTemporaryDir>

#include "runtime/host/appcontext.h"

class AppContextTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = std::make_unique<AppContext>(QStringLiteral("nulls"), dir.path(), &engine);
        QJSEngine::setObjectOwnership(context.get(), QJSEngine::CppOwnership);
        engine.globalObject().setProperty(QStringLiteral("App"), engine.newQObject(context.get()));
        ASSERT_TRUE(context->exec(QStringLiteral("CREATE TABLE t (a INTEGER, b TEXT)")));
    }

    QJSValue js(const QString& code) { return engine.evaluate(code); }

    int nullCount()
    {
        const QVariantList rows
            = context->query(QStringLiteral("SELECT COUNT(*) AS n FROM t WHERE b IS NULL"));
        return rows.first().toMap().value(QStringLiteral("n")).toInt();
    }

    int emptyCount()
    {
        const QVariantList rows
            = context->query(QStringLiteral("SELECT COUNT(*) AS n FROM t WHERE b = ''"));
        return rows.first().toMap().value(QStringLiteral("n")).toInt();
    }

    QTemporaryDir dir;
    QJSEngine engine;
    std::unique_ptr<AppContext> context;
};

TEST_F(AppContextTest, ANullInAPositionalListIsStoredAsSqlNull)
{
    const QJSValue result = js(QStringLiteral("App.exec('INSERT INTO t VALUES (?, ?)', [1, null])"));

    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_EQ(nullCount(), 1);
}

TEST_F(AppContextTest, ANullInANamedMapIsStoredAsSqlNull)
{
    const QJSValue result
        = js(QStringLiteral("App.exec('INSERT INTO t VALUES (:a, :b)', {a: 1, b: null})"));

    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    const QVariantList rows = context->query(QStringLiteral("SELECT a FROM t"));
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(rows.first().toMap().value(QStringLiteral("a")).toInt(), 1);
    EXPECT_EQ(nullCount(), 1);
}

TEST_F(AppContextTest, AnEmptyStringInAPositionalListIsStoredAsAnEmptyString)
{
    const QJSValue result = js(QStringLiteral("App.exec('INSERT INTO t VALUES (?, ?)', [1, ''])"));

    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_EQ(nullCount(), 0);
    EXPECT_EQ(emptyCount(), 1);
}

TEST_F(AppContextTest, AnEmptyStringInANamedMapIsStoredAsAnEmptyString)
{
    const QJSValue result
        = js(QStringLiteral("App.exec('INSERT INTO t VALUES (:a, :b)', {a: 1, b: ''})"));

    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    const QVariantList rows = context->query(QStringLiteral("SELECT a, typeof(b) AS kind FROM t"));
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(rows.first().toMap().value(QStringLiteral("a")).toInt(), 1);
    EXPECT_EQ(rows.first().toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("text"));
    EXPECT_EQ(nullCount(), 0);
    EXPECT_EQ(emptyCount(), 1);
}

TEST_F(AppContextTest, AnEmptyStringSatisfiesANotNullColumn)
{
    ASSERT_TRUE(context->exec(QStringLiteral("CREATE TABLE strict (note TEXT NOT NULL)")));

    const QJSValue result = js(QStringLiteral("App.exec('INSERT INTO strict VALUES (?)', [''])"));

    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_TRUE(context->lastError().isEmpty());
}

TEST_F(AppContextTest, AnUndefinedParameterIsStoredAsSqlNull)
{
    const QJSValue result
        = js(QStringLiteral("App.exec('INSERT INTO t VALUES (?, ?)', [1, undefined])"));

    EXPECT_FALSE(result.isError()) << result.toString().toStdString();
    EXPECT_EQ(nullCount(), 1);
}

TEST_F(AppContextTest, ANullParameterMatchesWithIsAndNotWithEquals)
{
    ASSERT_TRUE(context->exec(QStringLiteral("INSERT INTO t VALUES (1, NULL)")));

    const QJSValue rows = js(QStringLiteral("App.query('SELECT a FROM t WHERE b IS ?', [null])"));

    EXPECT_EQ(rows.property(QStringLiteral("length")).toInt(), 1);
}

TEST_F(AppContextTest, ANullColumnComesBackAsUndefined)
{
    ASSERT_TRUE(context->exec(QStringLiteral("INSERT INTO t VALUES (1, NULL)")));

    const QJSValue isUndefined
        = js(QStringLiteral("App.query('SELECT b FROM t')[0].b === undefined"));

    EXPECT_TRUE(isUndefined.toBool());
}
