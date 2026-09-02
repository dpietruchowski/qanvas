#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QQmlEngine>
#include <QTemporaryDir>

#include "runtime/app.h"
#include "runtime/appcompiler.h"

namespace
{
const QString kValidQml = QStringLiteral("import QtQuick\nItem {}\n");
const QString kUnclosedQml = QStringLiteral("import QtQuick\nItem {\n");

void writeFile(const QString& path, const QString& content)
{
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(content.toUtf8());
}
}

class AppCompilerTest : public ::testing::Test
{
protected:
    void SetUp() override { qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit()); }
    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
    QQmlEngine engine;
    AppCompiler compiler { engine };
    App app { QStringLiteral("notes") };
};

TEST_F(AppCompilerTest, AcceptsAnAppThatCompiles)
{
    writeFile(app.entryPath(), kValidQml);

    EXPECT_TRUE(compiler.compile(app).isSuccess());
}

TEST_F(AppCompilerTest, RefusesAnAppWithoutAnEntryFile)
{
    const Result<void> result = compiler.compile(app);

    ASSERT_TRUE(result.isFailure());
    EXPECT_EQ(result.error(), QStringLiteral("No app \"notes\""));
}

TEST_F(AppCompilerTest, NamesTheFileAndLineWithoutTheDirectory)
{
    writeFile(app.entryPath(), kUnclosedQml);

    const Result<void> result = compiler.compile(app);

    ASSERT_TRUE(result.isFailure());
    EXPECT_TRUE(result.error().startsWith(QStringLiteral("Main.qml:3")))
        << qPrintable(result.error());
    EXPECT_FALSE(result.error().contains(root.path())) << qPrintable(result.error());
}

TEST_F(AppCompilerTest, ReportsAComponentTheEntryFileCannotFind)
{
    writeFile(app.entryPath(), QStringLiteral("import QtQuick\nItem { Card {} }\n"));

    const Result<void> result = compiler.compile(app);

    ASSERT_TRUE(result.isFailure());
    EXPECT_TRUE(result.error().contains(QStringLiteral("Card"))) << qPrintable(result.error());
}

TEST_F(AppCompilerTest, SeesAFileChangedSinceTheLastCompile)
{
    writeFile(app.entryPath(), kUnclosedQml);
    ASSERT_TRUE(compiler.compile(app).isFailure());

    writeFile(app.entryPath(), kValidQml);

    EXPECT_TRUE(compiler.compile(app).isSuccess());
}

TEST_F(AppCompilerTest, SeesAComponentAddedBesideTheEntryFile)
{
    writeFile(app.entryPath(), QStringLiteral("import QtQuick\nItem { Card {} }\n"));
    ASSERT_TRUE(compiler.compile(app).isFailure());

    writeFile(app.dir() + QStringLiteral("/Card.qml"), kValidQml);

    EXPECT_TRUE(compiler.compile(app).isSuccess());
}
