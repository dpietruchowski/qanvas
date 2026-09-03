#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>

#include "runtime/app/appfiles.h"

namespace
{
const QString kMain = QStringLiteral("import QtQuick\n"
                                     "\n"
                                     "Item {\n"
                                     "    ListView {\n"
                                     "        model: notes\n"
                                     "        delegate: NoteRow {}\n"
                                     "    }\n"
                                     "\n"
                                     "    Text {\n"
                                     "        text: notes.length\n"
                                     "    }\n"
                                     "}\n");

const QString kRow = QStringLiteral("import QtQuick\n"
                                    "\n"
                                    "Text {\n"
                                    "    text: model.title\n"
                                    "}\n");

QString readAll(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}
}

class AppFilesTest : public ::testing::Test
{
protected:
    AppFilesTest()
        : files(dir.path())
    {
    }

    QString path(const QString& name) const { return dir.path() + QLatin1Char('/') + name; }

    QTemporaryDir dir;
    AppFiles files;
};

TEST_F(AppFilesTest, WritesAFileAndReportsItsSize)
{
    const Result<QString> written = files.write(QStringLiteral("Main.qml"), kMain);

    ASSERT_TRUE(written.isSuccess());
    EXPECT_EQ(written.value(), QStringLiteral("Main.qml 12L written"));
    EXPECT_EQ(readAll(path(QStringLiteral("Main.qml"))), kMain);
}

TEST_F(AppFilesTest, RefusesAnEmptyFile)
{
    EXPECT_TRUE(files.write(QStringLiteral("Main.qml"), QStringLiteral("  \n ")).isFailure());
}

TEST_F(AppFilesTest, ListsSourceFilesWithLineCounts)
{
    files.write(QStringLiteral("Main.qml"), kMain);
    files.write(QStringLiteral("NoteRow.qml"), kRow);
    QFile database(path(QStringLiteral("app.db")));
    ASSERT_TRUE(database.open(QIODevice::WriteOnly));
    database.close();

    const Result<QString> listed = files.list();

    ASSERT_TRUE(listed.isSuccess());
    EXPECT_EQ(listed.value(), QStringLiteral("Main.qml 12L\nNoteRow.qml 5L"));
}

TEST_F(AppFilesTest, OutlineNamesTheFunctionsOfEachFile)
{
    files.write(QStringLiteral("Data.qml"),
                QStringLiteral("import QtQuick\n"
                               "QtObject {\n"
                               "    function setup() {\n"
                               "        App.exec(\"CREATE TABLE IF NOT EXISTS items (id INTEGER "
                               "PRIMARY KEY)\")\n"
                               "    }\n"
                               "    function addItem( text,  done ) { return App.exec(\"...\") }\n"
                               "    property var functionLike: 1\n"
                               "}\n"));
    files.write(QStringLiteral("Main.qml"), kMain);
    files.write(QStringLiteral("helpers.js"), QStringLiteral("function pad(n) { return n }\n"));

    const Result<QString> outlined = files.outline();

    ASSERT_TRUE(outlined.isSuccess());
    EXPECT_EQ(outlined.value(),
              QStringLiteral("Data.qml 8L\n"
                             "  setup()\n"
                             "  addItem(text, done)\n"
                             "Main.qml 12L\n"
                             "helpers.js 1L\n"
                             "  pad(n)"));
}

TEST_F(AppFilesTest, ReadsTheWholeFileWithLineNumbers)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> content = files.read(QStringLiteral("Main.qml"));

    ASSERT_TRUE(content.isSuccess());
    EXPECT_EQ(content.value(),
              QStringLiteral("Main.qml 12L\n"
                             "   1: import QtQuick\n"
                             "   2:\n"
                             "   3: Item {\n"
                             "   4:     ListView {\n"
                             "   5:         model: notes\n"
                             "   6:         delegate: NoteRow {}\n"
                             "   7:     }\n"
                             "   8:\n"
                             "   9:     Text {\n"
                             "  10:         text: notes.length\n"
                             "  11:     }\n"
                             "  12: }"));
}

TEST_F(AppFilesTest, ReadsARange)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> content = files.read(QStringLiteral("Main.qml"), 4, 2);

    ASSERT_TRUE(content.isSuccess());
    EXPECT_EQ(content.value(),
              QStringLiteral("Main.qml 12L\n"
                             "   4:     ListView {\n"
                             "   5:         model: notes"));
}

TEST_F(AppFilesTest, ReadingPastTheEndSaysHowLongTheFileIs)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> content = files.read(QStringLiteral("Main.qml"), 99);

    ASSERT_TRUE(content.isFailure());
    EXPECT_EQ(content.error(), QStringLiteral("Main.qml has 12 lines"));
}

TEST_F(AppFilesTest, ReadingAMissingFileListsWhatIsThere)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> content = files.read(QStringLiteral("NoteRow.qml"));

    ASSERT_TRUE(content.isFailure());
    EXPECT_EQ(content.error(), QStringLiteral("No NoteRow.qml — files: Main.qml"));
}

TEST_F(AppFilesTest, GrepShowsTheLinesAroundEachHit)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> found = files.grep(QStringLiteral("model:"), 1);

    ASSERT_TRUE(found.isSuccess());
    const QStringList lines = found.value().split(QLatin1Char('\n'));
    ASSERT_EQ(lines.size(), 4);
    EXPECT_EQ(lines.at(0), QStringLiteral("Main.qml"));
    EXPECT_TRUE(lines.at(1).startsWith(QStringLiteral("   4 ")));
    EXPECT_TRUE(lines.at(2).startsWith(QStringLiteral("   5>")));
    EXPECT_TRUE(lines.at(3).startsWith(QStringLiteral("   6 ")));
}

TEST_F(AppFilesTest, GrepSpansEveryFileAndNamesEachOne)
{
    files.write(QStringLiteral("Main.qml"), kMain);
    files.write(QStringLiteral("NoteRow.qml"), kRow);

    const Result<QString> found = files.grep(QStringLiteral("import QtQuick"), 0);

    ASSERT_TRUE(found.isSuccess());
    EXPECT_EQ(found.value(),
              QStringLiteral("Main.qml\n"
                             "   1> import QtQuick\n"
                             "\n"
                             "NoteRow.qml\n"
                             "   1> import QtQuick"));
}

TEST_F(AppFilesTest, GrepSaysWhenNothingMatches)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> found = files.grep(QStringLiteral("Repeater"));

    ASSERT_TRUE(found.isSuccess());
    EXPECT_EQ(found.value(), QStringLiteral("No matches for \"Repeater\""));
}

TEST_F(AppFilesTest, GrepCanBeLimitedToOneFile)
{
    files.write(QStringLiteral("Main.qml"), kMain);
    files.write(QStringLiteral("NoteRow.qml"), kRow);

    const Result<QString> found
        = files.grep(QStringLiteral("import"), 0, QStringLiteral("NoteRow.qml"));

    ASSERT_TRUE(found.isSuccess());
    EXPECT_FALSE(found.value().contains(QStringLiteral("Main.qml")));
}

TEST_F(AppFilesTest, ReplacesOnAUniqueAnchorAndShowsTheChange)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> changed = files.replace(
        QStringLiteral("Main.qml"), QStringLiteral("model: notes"),
        QStringLiteral("model: (App.revision, App.query(\"SELECT * FROM notes\"))"));

    ASSERT_TRUE(changed.isSuccess());
    const QStringList lines = changed.value().split(QLatin1Char('\n'));
    ASSERT_EQ(lines.size(), 4);
    EXPECT_EQ(lines.at(0), QStringLiteral("Main.qml 1 replacement"));
    EXPECT_TRUE(lines.at(2).startsWith(QStringLiteral("   5>")));
    EXPECT_TRUE(lines.at(2).contains(QStringLiteral("App.query")));
    EXPECT_TRUE(readAll(path(QStringLiteral("Main.qml"))).contains(QStringLiteral("App.query")));
}

TEST_F(AppFilesTest, RefusesAnAmbiguousAnchorAndPointsAtTheMatches)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> changed = files.replace(QStringLiteral("Main.qml"),
                                                  QStringLiteral("notes"), QStringLiteral("tasks"));

    ASSERT_TRUE(changed.isFailure());
    EXPECT_EQ(changed.error(), QStringLiteral("2 matches at lines 5, 10 — extend the anchor"));
    EXPECT_EQ(readAll(path(QStringLiteral("Main.qml"))), kMain);
}

TEST_F(AppFilesTest, ReplacesEveryOccurrenceWhenAsked)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> changed = files.replace(
        QStringLiteral("Main.qml"), QStringLiteral("notes"), QStringLiteral("tasks"), true);

    ASSERT_TRUE(changed.isSuccess());
    EXPECT_TRUE(changed.value().startsWith(QStringLiteral("Main.qml 2 replacements")));
    EXPECT_FALSE(readAll(path(QStringLiteral("Main.qml"))).contains(QStringLiteral("notes")));
}

TEST_F(AppFilesTest, SaysWhenTheAnchorIsNotThere)
{
    files.write(QStringLiteral("Main.qml"), kMain);

    const Result<QString> changed = files.replace(
        QStringLiteral("Main.qml"), QStringLiteral("Repeater"), QStringLiteral("Row"));

    ASSERT_TRUE(changed.isFailure());
    EXPECT_EQ(changed.error(), QStringLiteral("Anchor not found in Main.qml"));
}

TEST_F(AppFilesTest, RemovesAComponentButNotTheEntryPoint)
{
    files.write(QStringLiteral("Main.qml"), kMain);
    files.write(QStringLiteral("NoteRow.qml"), kRow);

    EXPECT_TRUE(files.remove(QStringLiteral("Main.qml")).isFailure());
    EXPECT_TRUE(QFile::exists(path(QStringLiteral("Main.qml"))));

    const Result<QString> removed = files.remove(QStringLiteral("NoteRow.qml"));

    ASSERT_TRUE(removed.isSuccess());
    EXPECT_EQ(removed.value(), QStringLiteral("NoteRow.qml removed"));
    EXPECT_FALSE(QFile::exists(path(QStringLiteral("NoteRow.qml"))));
}

TEST_F(AppFilesTest, KeepsEveryOperationInsideTheApp)
{
    const QStringList rejected { QStringLiteral(""),
                                 QStringLiteral("../escape.qml"),
                                 QStringLiteral("sub/Main.qml"),
                                 QStringLiteral("/etc/passwd"),
                                 QStringLiteral("app.db"),
                                 QStringLiteral("icon.svg"),
                                 QStringLiteral(".hidden.qml"),
                                 QStringLiteral("Main.qml.bak") };

    for (const QString& name : rejected)
    {
        EXPECT_TRUE(files.write(name, kMain).isFailure()) << name.toStdString();
        EXPECT_TRUE(files.read(name).isFailure()) << name.toStdString();
        EXPECT_TRUE(files.remove(name).isFailure()) << name.toStdString();
        EXPECT_TRUE(files.replace(name, QStringLiteral("a"), QStringLiteral("b")).isFailure())
            << name.toStdString();
    }

    EXPECT_TRUE(files.list().isFailure());
}
