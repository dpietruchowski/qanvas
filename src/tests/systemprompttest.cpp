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
                              "list_apps", "list_files", "read_file", "search_files",
                              "describe_app", "read_log", "check_layout", "grant_access" })
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
           "QanvasProgress", "QanvasStepper", "QanvasPanel", "emptyText", "QanvasChart" })
        EXPECT_TRUE(prompt.contains(QLatin1String(component))) << component;
}

TEST(SystemPromptTest, SaysWhereToLookBeforeBuildingASecondApp)
{
    const QString prompt = SystemPrompt::text();
    const qsizetype start = prompt.indexOf(QStringLiteral("## STATE: DESIGN"));
    const qsizetype end = prompt.indexOf(QStringLiteral("## STATE: BUILD SMALL"));
    ASSERT_GE(start, 0);
    ASSERT_GT(end, start);
    const QString design = prompt.mid(start, end - start);
    EXPECT_TRUE(design.contains(QStringLiteral("list_apps")));
    EXPECT_TRUE(design.contains(QStringLiteral("create_app replaces the app of that id")));
}

TEST(SystemPromptTest, ACategoryColourIsKeptAsATypeWord)
{
    const QString prompt = SystemPrompt::text();
    EXPECT_TRUE(prompt.contains(QStringLiteral("is kept as one of\nthe words primary|success|warning|danger")));
    EXPECT_TRUE(prompt.contains(QStringLiteral("QanvasSelect { options: [\"primary\", \"success\", \"warning\", \"danger\"] }")));
    EXPECT_TRUE(prompt.contains(QStringLiteral("(no type: the neutral grey one)")));
}

TEST(SystemPromptTest, TheDataSectionSaysHowNullAndAnEmptyStringBind)
{
    const QString prompt = SystemPrompt::text();
    EXPECT_TRUE(prompt.contains(QStringLiteral("null and undefined are stored as NULL")));
    EXPECT_TRUE(prompt.contains(QStringLiteral("\"\" stays an\nempty string")));
    EXPECT_TRUE(prompt.contains(QStringLiteral("IS NULL or\nIS ?, never = ?")));
}

TEST(SystemPromptTest, TheSelectShowsItsPlaceholderForAnEmptyValue)
{
    const QString prompt = SystemPrompt::text();
    EXPECT_TRUE(prompt.contains(QStringLiteral("QanvasSelect{options value placeholder onSelected(value)}")));
    EXPECT_TRUE(prompt.contains(QStringLiteral("an empty or unknown value shows placeholder")));
}

TEST(SystemPromptTest, TheStepperSaysNumbersCarryNoThousandsSeparator)
{
    EXPECT_TRUE(SystemPrompt::text().contains(QStringLiteral("it reads 2026, never 2,026")));
}

TEST(SystemPromptTest, NamesEveryCataloguePropertyAndTypeWord)
{
    const QString prompt = SystemPrompt::text();
    for (const char* word :
         { "QanvasButton{text iconName onClicked type enabled}", "enabled: false",
           "(primary|secondary|danger|success|warning|ghost)", "badgeRole",
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
