#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QtQml>

#include "runtime/host/applayout.h"
#include "runtime/host/apploader.h"
#include "runtime/host/appscreen.h"

namespace
{
class AppScreenTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        qputenv("QANVAS_APPS_ROOT", root().path().toLocal8Bit());
        qmlRegisterType<AppLoader>("ScreenHost", 1, 0, "AppLoader");
    }

    static void TearDownTestSuite() { qunsetenv("QANVAS_APPS_ROOT"); }

    static QTemporaryDir& root()
    {
        static QTemporaryDir directory;
        return directory;
    }

    void write(const QString& id, const QString& name, const QString& body)
    {
        ASSERT_TRUE(QDir().mkpath(root().path() + QLatin1Char('/') + id));
        QFile file(root().path() + QLatin1Char('/') + id + QLatin1Char('/') + name);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(body.toUtf8());
    }

    void writeScreens(const QString& id)
    {
        write(id, QStringLiteral("Main.qml"),
              QStringLiteral("import QtQuick\nItem {\n    property string screen: \"first\"\n"
                             "    Loader {\n        anchors.fill: parent\n"
                             "        source: screen === \"first\" ? \"First.qml\" : "
                             "\"Second.qml\"\n    }\n}\n"));
        write(id, QStringLiteral("First.qml"),
              QStringLiteral("import QtQuick\nItem { Text { text: \"first\" } }\n"));
        write(id, QStringLiteral("Second.qml"),
              QStringLiteral("import QtQuick\nItem { Text { text: \"second\" } }\n"));
    }

    void show(const QString& id)
    {
        const QString path = root().path() + QStringLiteral("/Host.qml");
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(QStringLiteral("import QtQuick\nimport ScreenHost\nItem {\n"
                                  "    width: 300; height: 400\n"
                                  "    AppLoader { anchors.fill: parent; appId: \"%1\" }\n}\n")
                       .arg(id)
                       .toUtf8());
        file.close();

        view.resize(300, 400);
        view.setSource(QUrl::fromLocalFile(path));
        ASSERT_EQ(view.status(), QQuickView::Ready);
        view.show();
        ASSERT_TRUE(QTest::qWaitForWindowExposed(&view));
        QTest::qWait(50);
    }

    QQuickView view;
};
}

TEST_F(AppScreenTest, MeasuresTheRequestedScreenAndPutsTheOpenOneBack)
{
    writeScreens(QStringLiteral("screens"));
    show(QStringLiteral("screens"));
    QQuickItem* root = AppLoader::running(QStringLiteral("screens"))->item();
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(AppLayout::measureApp(QStringLiteral("screens")).files,
              QStringLiteral("First.qml, Main.qml"));

    {
        const AppScreen other(QStringLiteral("screens"), QStringLiteral("second"));

        EXPECT_TRUE(other.error().isEmpty());
        EXPECT_EQ(root->property("screen").toString(), QStringLiteral("second"));
        EXPECT_EQ(AppLayout::measureApp(QStringLiteral("screens")).files,
                  QStringLiteral("Main.qml, Second.qml"));
    }

    EXPECT_EQ(root->property("screen").toString(), QStringLiteral("first"));
}

TEST_F(AppScreenTest, AnEmptyScreenLeavesTheOpenOneAlone)
{
    writeScreens(QStringLiteral("empty"));
    show(QStringLiteral("empty"));
    QQuickItem* root = AppLoader::running(QStringLiteral("empty"))->item();
    ASSERT_NE(root, nullptr);

    const AppScreen same(QStringLiteral("empty"), QString());

    EXPECT_TRUE(same.error().isEmpty());
    EXPECT_EQ(root->property("screen").toString(), QStringLiteral("first"));
}

TEST_F(AppScreenTest, AnAppWithoutAScreenPropertySaysSo)
{
    write(QStringLiteral("plain"), QStringLiteral("Main.qml"),
          QStringLiteral("import QtQuick\nItem { Text { text: \"plain\" } }\n"));
    show(QStringLiteral("plain"));

    const AppScreen other(QStringLiteral("plain"), QStringLiteral("second"));

    EXPECT_TRUE(other.error().contains(QStringLiteral("no screen property")))
        << qPrintable(other.error());
}

TEST_F(AppScreenTest, AnAppThatIsNotOnScreenIsLeftAlone)
{
    const AppScreen other(QStringLiteral("nowhere"), QStringLiteral("second"));

    EXPECT_TRUE(other.error().isEmpty());
}
