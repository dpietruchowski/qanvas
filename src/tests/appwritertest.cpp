#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "runtime/appwriter.h"

namespace
{
const QString kApp = QStringLiteral("import QtQuick\n\nItem {\n}\n");

QString readAll(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}
}

class AppWriterTest : public ::testing::Test
{
protected:
    void SetUp() override { qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit()); }
    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
    QString error;
};

TEST_F(AppWriterTest, RootFollowsTheEnvironment)
{
    EXPECT_EQ(AppWriter::appsRoot(), root.path());
    EXPECT_EQ(AppWriter::appDir(QStringLiteral("notes")), root.path() + QStringLiteral("/notes"));
}

TEST_F(AppWriterTest, WritesTheAppUnderItsOwnDirectory)
{
    const QString path = AppWriter::write(QStringLiteral("notes"), kApp, &error);

    EXPECT_EQ(path, root.path() + QStringLiteral("/notes/Main.qml"));
    EXPECT_TRUE(error.isEmpty());
    EXPECT_EQ(readAll(path), kApp);
}

TEST_F(AppWriterTest, RewritingAnAppReplacesIt)
{
    AppWriter::write(QStringLiteral("notes"), kApp, &error);
    const QString second = QStringLiteral("import QtQuick\n\nRectangle {\n}\n");

    const QString path = AppWriter::write(QStringLiteral("notes"), second, &error);

    EXPECT_TRUE(error.isEmpty());
    EXPECT_EQ(readAll(path), second);
}

TEST_F(AppWriterTest, RejectsAnIdThatIsNotASlug)
{
    const QStringList rejected { QStringLiteral(""),         QStringLiteral("../escape"),
                                 QStringLiteral("a/b"),      QStringLiteral("Notes"),
                                 QStringLiteral("-notes"),   QStringLiteral("no tes"),
                                 QStringLiteral("notes.db"), QString(33, QLatin1Char('a')) };

    for (const QString& id : rejected)
    {
        error.clear();
        EXPECT_FALSE(AppWriter::isValidId(id)) << id.toStdString();
        EXPECT_TRUE(AppWriter::write(id, kApp, &error).isEmpty()) << id.toStdString();
        EXPECT_FALSE(error.isEmpty()) << id.toStdString();
    }

    EXPECT_TRUE(QDir(root.path()).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty());
}

TEST_F(AppWriterTest, RejectsAnEmptyApp)
{
    EXPECT_TRUE(
        AppWriter::write(QStringLiteral("notes"), QStringLiteral("  \n "), &error).isEmpty());
    EXPECT_FALSE(error.isEmpty());
}

TEST_F(AppWriterTest, AcceptsSlugsWithDigitsAndDashes)
{
    EXPECT_TRUE(AppWriter::isValidId(QStringLiteral("water-plants-2")));
    EXPECT_FALSE(AppWriter::write(QStringLiteral("water-plants-2"), kApp, &error).isEmpty());
    EXPECT_TRUE(error.isEmpty());
}
