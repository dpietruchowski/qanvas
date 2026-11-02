#include <gtest/gtest.h>

#include <QRegularExpression>
#include <QSet>

#include "runtime/chat/systemprompt.h"

namespace
{
QSet<QString> matches(const QString& text, const QString& pattern)
{
    QSet<QString> found;
    QRegularExpressionMatchIterator it = QRegularExpression(pattern).globalMatch(text);
    while (it.hasNext())
        found.insert(it.next().captured(1));
    return found;
}
}

TEST(SystemPromptTest, LoadsFromTheResource) { EXPECT_FALSE(SystemPrompt::text().isEmpty()); }

TEST(SystemPromptTest, NamesEveryTool)
{
    const QString prompt = SystemPrompt::text();
    for (const char* tool : { "create_app", "write_file", "edit_file", "delete_file", "delete_app",
                              "list_files", "read_file", "search_files", "describe_app", "read_log",
                              "check_layout", "grant_access" })
        EXPECT_TRUE(prompt.contains(QLatin1String(tool))) << tool;
    EXPECT_TRUE(prompt.contains(QStringLiteral("\nnote   "))) << "note";
}

TEST(SystemPromptTest, EveryPathThatChangesAnAppEndsInTheNoteBeforeDone)
{
    const QStringList sections = SystemPrompt::text().split(QStringLiteral("\n## "));
    int checked = 0;
    for (const QString& section : sections)
    {
        const QString name = section.section(QLatin1Char('\n'), 0, 0);
        if (name.startsWith(QStringLiteral("STATE: NOTE")))
        {
            EXPECT_TRUE(section.contains(QStringLiteral("-> DONE")));
            continue;
        }
        if (!name.startsWith(QStringLiteral("STATE: ")))
            continue;
        EXPECT_FALSE(section.contains(QStringLiteral("-> DONE"))) << qPrintable(name);
        for (const char* state : { "BUILD", "MODIFY", "FIX", "DEBUG" })
        {
            if (name.startsWith(QStringLiteral("STATE: ") + QLatin1String(state)))
            {
                EXPECT_TRUE(section.contains(QStringLiteral("NOTE"))) << qPrintable(name);
                ++checked;
            }
        }
    }
    EXPECT_EQ(checked, 5);
}

TEST(SystemPromptTest, EveryExampleThatEndsInDoneWritesTheNoteFirst)
{
    const QString prompt = SystemPrompt::text();
    const qsizetype examples = prompt.indexOf(QStringLiteral("## EXAMPLE 1"));
    ASSERT_GE(examples, 0);
    const QStringList parts = prompt.mid(examples).split(QStringLiteral("\nDONE:"));
    ASSERT_EQ(parts.size(), 3);
    EXPECT_TRUE(parts[0].contains(QStringLiteral("\nnote  app=")));
    EXPECT_TRUE(parts[1].contains(QStringLiteral("note  app=")));
}

TEST(SystemPromptTest, NamesEveryComponent)
{
    const QString prompt = SystemPrompt::text();
    for (const char* component :
         { "QanvasCountdown", "QanvasStopwatch", "QanvasStat", "QanvasBadge", "QanvasTabs",
           "QanvasProgress", "QanvasStepper", "QanvasPanel", "emptyText" })
        EXPECT_TRUE(prompt.contains(QLatin1String(component))) << component;
}

TEST(SystemPromptTest, SaysHowAScrollLaysOutItsChildren)
{
    const QString prompt = SystemPrompt::text();
    const qsizetype line = prompt.indexOf(QStringLiteral("QanvasScroll  scrolls"));
    ASSERT_GE(line, 0);
    const QString entry = prompt.mid(line, 220);
    EXPECT_TRUE(entry.contains(QStringLiteral("stacked as a column")));
    EXPECT_TRUE(entry.contains(QStringLiteral("Layout.fillWidth: true")));
    EXPECT_TRUE(entry.contains(QStringLiteral("Repeater")));
}

TEST(SystemPromptTest, SaysWhereAToastGoes)
{
    const QString prompt = SystemPrompt::text();
    const qsizetype line = prompt.indexOf(QStringLiteral("QanvasToast{show(text)}"));
    ASSERT_GE(line, 0);
    const QString entry = prompt.mid(line, 200);
    EXPECT_TRUE(entry.contains(QStringLiteral("on the page itself, outside any column or row")));
    EXPECT_TRUE(entry.contains(QStringLiteral("needs no size")));
}

TEST(SystemPromptTest, NamesEveryCataloguePropertyAndTypeWord)
{
    const QString prompt = SystemPrompt::text();
    for (const char* word :
         { "QanvasButton{text iconName onClicked type enabled}", "enabled: false", "badgeRole",
           "typeRole", "badgeFormat", "typeFormat", "\"done\"",
           "QanvasProgress{value from to type:success|warning|danger}", "type: \"centiseconds\"",
           "centiseconds", "m:ss.cc" })
        EXPECT_TRUE(prompt.contains(QLatin1String(word))) << word;
}

TEST(SystemPromptTest, TabsSayTheyCarryTheOptionTextNotTheIndex)
{
    const QString prompt = SystemPrompt::text();
    EXPECT_TRUE(prompt.contains(QStringLiteral("carry the text of")));
    EXPECT_TRUE(prompt.contains(QStringLiteral("never its index")));
}

TEST(SystemPromptTest, TheLayoutSectionPointsAtTheCheckboxRow)
{
    const QString prompt = SystemPrompt::text();
    const qsizetype start = prompt.indexOf(QStringLiteral("\n## LAYOUT"));
    const qsizetype end = prompt.indexOf(QStringLiteral("\n## SCREENS"));
    ASSERT_GE(start, 0);
    ASSERT_GT(end, start);
    const QString layout = prompt.mid(start, end - start);
    EXPECT_TRUE(layout.contains(QStringLiteral("QanvasListItem with\n    checkable")));
    EXPECT_TRUE(layout.contains(QStringLiteral("QanvasList has no checked column")));
}

TEST(SystemPromptTest, TheLayoutProblemsEntryNamesWhatTheMeasurementReports)
{
    const QString prompt = SystemPrompt::text();
    const qsizetype start = prompt.indexOf(QStringLiteral("\"Layout problems\""));
    const qsizetype end = prompt.indexOf(QStringLiteral("\"Rejected and undone\""));
    ASSERT_GE(start, 0);
    ASSERT_GT(end, start);
    const QString entry = prompt.mid(start, end - start).simplified();
    EXPECT_TRUE(
        entry.contains(QStringLiteral("cut off (by the screen, by a card or scroll around it")));
    EXPECT_TRUE(entry.contains(QStringLiteral("a text that does not fit")));
    EXPECT_TRUE(entry.contains(QStringLiteral("is drawn on top of another element")));
    EXPECT_TRUE(entry.contains(QStringLiteral("names the piece and the pixels")));
    EXPECT_TRUE(entry.contains(QStringLiteral("QanvasColumn or QanvasRow")));
}

TEST(SystemPromptTest, TheCheckLayoutToolListsEveryKindOfLayoutProblem)
{
    const QString prompt = SystemPrompt::text();
    const qsizetype line = prompt.indexOf(QStringLiteral("check_layout   measures the app"));
    ASSERT_GE(line, 0);
    const QString entry = prompt.mid(line, 260).simplified();
    EXPECT_TRUE(entry.contains(QStringLiteral("what sticks out")));
    EXPECT_TRUE(entry.contains(QStringLiteral("is cut off (also a text that does not fit)")));
    EXPECT_TRUE(entry.contains(QStringLiteral("is drawn over another element")));
    EXPECT_TRUE(entry.contains(QStringLiteral("has no size")));
    EXPECT_TRUE(entry.contains(QStringLiteral("the reload tools include the same lines")));
}

TEST(SystemPromptTest, TheTimeRuleNamesTheSubSecondStopwatch)
{
    const QString prompt = SystemPrompt::text();
    EXPECT_TRUE(prompt.contains(QStringLiteral("Hundredths of a second are a\nQanvasStopwatch")));
    EXPECT_FALSE(prompt.contains(QStringLiteral("does not fit a property int")));
}

TEST(SystemPromptTest, EveryTransitionLeadsToADefinedState)
{
    const QString prompt = SystemPrompt::text();
    const QSet<QString> defined = matches(prompt, QStringLiteral("STATE: ([A-Z]+)"));
    const QSet<QString> targets = matches(prompt, QStringLiteral("-> ([A-Z]{3,})\\b"));

    EXPECT_TRUE(defined.contains(QStringLiteral("ROUTE")));
    EXPECT_TRUE(defined.contains(QStringLiteral("DESIGN")));
    EXPECT_TRUE(defined.contains(QStringLiteral("BUILD")));
    EXPECT_TRUE(defined.contains(QStringLiteral("INSPECT")));
    EXPECT_TRUE(defined.contains(QStringLiteral("MODIFY")));
    EXPECT_TRUE(defined.contains(QStringLiteral("FIX")));
    EXPECT_TRUE(defined.contains(QStringLiteral("DONE")));
    for (const QString& target : targets)
        EXPECT_TRUE(defined.contains(target)) << target.toStdString();
}
