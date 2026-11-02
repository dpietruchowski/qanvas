#include <gtest/gtest.h>

#include <QQmlEngine>
#include <QStringList>

#include "runtime/app/appicons.h"
#include "runtime/app/appregistry.h"
#include "themedtypes.h"

class AppIconsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        prepareThemedEngine(engine);
        engine.setOutputWarningsToStandardError(false);
    }

    QQmlEngine engine;
};

TEST_F(AppIconsTest, NamesAreTheIconsOfTheTheme)
{
    const AppIcons icons(&engine);

    const QStringList names = icons.names();
    EXPECT_GT(names.size(), 70);
    EXPECT_TRUE(names.contains(QStringLiteral("cart")));
    EXPECT_TRUE(names.contains(QStringLiteral("star")));
}

TEST_F(AppIconsTest, AnIconNameResolvesToItself)
{
    const AppIcons icons(&engine);

    EXPECT_EQ(icons.resolve(QStringLiteral("wallet")), QStringLiteral("wallet"));
}

TEST_F(AppIconsTest, AnAliasResolvesToTheIconItStandsFor)
{
    const AppIcons icons(&engine);

    EXPECT_EQ(icons.resolve(QStringLiteral("trash")), QStringLiteral("remove"));
}

TEST_F(AppIconsTest, ANameThatIsNoIconResolvesToNothing)
{
    const AppIcons icons(&engine);

    EXPECT_TRUE(icons.resolve(QStringLiteral("rocket")).isEmpty());
    EXPECT_TRUE(icons.resolve(QString()).isEmpty());
    EXPECT_TRUE(icons.resolve(QStringLiteral("constructor")).isEmpty());
}

TEST_F(AppIconsTest, WithoutAnEngineNothingResolves)
{
    const AppIcons icons(nullptr);

    EXPECT_TRUE(icons.names().isEmpty());
    EXPECT_TRUE(icons.resolve(QStringLiteral("cart")).isEmpty());
}

TEST_F(AppIconsTest, TheDefaultIconOfTheRegistryIsAnIconOfTheTheme)
{
    const AppIcons icons(&engine);

    EXPECT_EQ(icons.resolve(AppRegistry::defaultIcon()), AppRegistry::defaultIcon());
}
