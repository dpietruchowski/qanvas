#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "runtime/app/app.h"

class AppTest : public ::testing::Test
{
protected:
    void SetUp() override { qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit()); }
    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
};

TEST_F(AppTest, RootFollowsTheEnvironment) { EXPECT_EQ(App::root(), root.path()); }

TEST_F(AppTest, RootSitsNextToTheBinaryWithoutTheEnvironment)
{
    qunsetenv("QANVAS_APPS_ROOT");

    EXPECT_EQ(App::root(), QCoreApplication::applicationDirPath() + QStringLiteral("/apps"));
}

TEST_F(AppTest, AddressesTheFilesOfOneApp)
{
    const App app(QStringLiteral("notes"));

    EXPECT_EQ(app.id(), QStringLiteral("notes"));
    EXPECT_EQ(app.dir(), root.path() + QStringLiteral("/notes"));
    EXPECT_EQ(app.entryPath(), root.path() + QStringLiteral("/notes/Main.qml"));
    EXPECT_EQ(app.databasePath(), root.path() + QStringLiteral("/notes/app.db"));
    EXPECT_EQ(app.iconPath(), root.path() + QStringLiteral("/notes/icon.svg"));
}

TEST_F(AppTest, RejectsAnIdThatIsNotASlug)
{
    const QStringList rejected { QStringLiteral(""),         QStringLiteral("../escape"),
                                 QStringLiteral("a/b"),      QStringLiteral("Notes"),
                                 QStringLiteral("-notes"),   QStringLiteral("no tes"),
                                 QStringLiteral("notes.db"), QString(33, QLatin1Char('a')) };

    for (const QString& id : rejected)
    {
        EXPECT_FALSE(App::isValidId(id)) << id.toStdString();
        EXPECT_FALSE(App(id).isValid()) << id.toStdString();
        EXPECT_TRUE(App(id).dir().isEmpty()) << id.toStdString();
        EXPECT_TRUE(App(id).entryPath().isEmpty()) << id.toStdString();
    }
}

TEST_F(AppTest, AcceptsSlugsWithDigitsAndDashes)
{
    EXPECT_TRUE(App::isValidId(QStringLiteral("water-plants-2")));
    EXPECT_EQ(App(QStringLiteral("water-plants-2")).dir(),
              root.path() + QStringLiteral("/water-plants-2"));
}

TEST_F(AppTest, ExistsOnlyOnceTheEntryFileIsThere)
{
    const App app(QStringLiteral("notes"));
    EXPECT_FALSE(app.exists());

    ASSERT_TRUE(QDir().mkpath(app.dir()));
    QFile file(app.entryPath());
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("import QtQuick\nItem {}\n");
    file.close();

    EXPECT_TRUE(app.exists());
    EXPECT_FALSE(App(QStringLiteral("Notes")).exists());
}
