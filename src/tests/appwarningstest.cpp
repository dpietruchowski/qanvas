#include <gtest/gtest.h>

#include <memory>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QUrl>

#include "runtime/app/app.h"
#include "runtime/app/appwarnings.h"

namespace
{
const QString kThrowingQml
    = QStringLiteral("import QtQuick\nItem {\n    Component.onCompleted: undefined.foo\n}\n");

void writeFile(const QString& path, const QString& content)
{
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(content.toUtf8());
}
}

class AppWarningsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit());
        engine.setOutputWarningsToStandardError(false);
    }

    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    std::unique_ptr<QObject> run(const QString& path)
    {
        engine.clearComponentCache();
        QQmlComponent component(&engine, QUrl::fromLocalFile(path));
        return std::unique_ptr<QObject>(component.create());
    }

    QTemporaryDir root;
    QTemporaryDir elsewhere;
    QQmlEngine engine;
    AppWarnings warnings { engine };
    App app { QStringLiteral("notes") };
};

TEST_F(AppWarningsTest, FilesARuntimeErrorUnderTheAppItCameFrom)
{
    writeFile(app.entryPath(), kThrowingQml);

    const std::unique_ptr<QObject> running = run(app.entryPath());

    const QStringList lines = warnings.lines(app.id());
    ASSERT_EQ(lines.size(), 1);
    EXPECT_TRUE(lines.first().startsWith(QStringLiteral("Main.qml:3")))
        << qPrintable(lines.first());
    EXPECT_TRUE(lines.first().contains(QStringLiteral("TypeError"))) << qPrintable(lines.first());
    EXPECT_FALSE(lines.first().contains(root.path())) << qPrintable(lines.first());
}

TEST_F(AppWarningsTest, KeepsAppsApart)
{
    const App timer(QStringLiteral("timer"));
    writeFile(app.entryPath(), kThrowingQml);
    writeFile(timer.entryPath(), QStringLiteral("import QtQuick\nItem {}\n"));

    const std::unique_ptr<QObject> notes = run(app.entryPath());
    const std::unique_ptr<QObject> other = run(timer.entryPath());

    EXPECT_EQ(warnings.lines(app.id()).size(), 1);
    EXPECT_TRUE(warnings.lines(timer.id()).isEmpty());
}

TEST_F(AppWarningsTest, IgnoresFilesOutsideTheAppsRoot)
{
    const QString path = elsewhere.path() + QStringLiteral("/notes/Main.qml");
    writeFile(path, kThrowingQml);

    const std::unique_ptr<QObject> running = run(path);

    EXPECT_TRUE(warnings.lines(app.id()).isEmpty());
}

TEST_F(AppWarningsTest, ClearForgetsOneApp)
{
    writeFile(app.entryPath(), kThrowingQml);
    const std::unique_ptr<QObject> running = run(app.entryPath());
    ASSERT_EQ(warnings.lines(app.id()).size(), 1);

    warnings.clear(app.id());

    EXPECT_TRUE(warnings.lines(app.id()).isEmpty());
}

TEST_F(AppWarningsTest, KeepsOnlyTheNewestFiftyLines)
{
    writeFile(app.entryPath(), kThrowingQml);

    for (int i = 0; i < 55; ++i)
        run(app.entryPath());

    EXPECT_EQ(warnings.lines(app.id()).size(), 50);
}
