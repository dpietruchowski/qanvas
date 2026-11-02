#include <gtest/gtest.h>

#include <QRegularExpression>
#include <QSet>
#include <QStringList>

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

QString sectionOf(const QString& text, const QString& header)
{
    const qsizetype start = text.indexOf(header);
    if (start < 0)
        return QString();
    const qsizetype next = text.indexOf(QStringLiteral("\n## "), start + header.size());
    return text.mid(start, (next < 0 ? text.size() : next) - start).simplified();
}

QString mainStage(const QString& stage)
{
    const QString text = SystemPrompt::mainText();
    const QStringList stages { QStringLiteral("ASK"), QStringLiteral("PLAN"),
                               QStringLiteral("DELEGATE"), QStringLiteral("REPORT") };
    const qsizetype start = text.indexOf(QStringLiteral("\n") + stage + QStringLiteral("\n"));
    if (start < 0)
        return QString();
    qsizetype stop = text.size();
    for (const QString& other : stages)
    {
        const qsizetype at
            = text.indexOf(QStringLiteral("\n") + other + QStringLiteral("\n"), start + 1);
        if (at >= 0 && at < stop)
            stop = at;
    }
    return text.mid(start, stop - start).simplified();
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
        for (const char* state : { "BUILD", "MODIFY", "STEPS", "FIX", "DEBUG" })
        {
            if (name.startsWith(QStringLiteral("STATE: ") + QLatin1String(state)))
            {
                EXPECT_TRUE(section.contains(QStringLiteral("NOTE"))) << qPrintable(name);
                ++checked;
            }
        }
    }
    EXPECT_EQ(checked, 6);
}

TEST(SystemPromptTest, EveryExampleThatEndsInDoneWritesTheNoteFirst)
{
    const QString prompt = SystemPrompt::text();
    const qsizetype examples = prompt.indexOf(QStringLiteral("## EXAMPLE 1"));
    ASSERT_GE(examples, 0);
    const QStringList parts = prompt.mid(examples).split(QStringLiteral("\nDONE:"));
    ASSERT_EQ(parts.size(), 4);
    EXPECT_TRUE(parts[0].contains(QStringLiteral("\nnote  app=")));
    EXPECT_TRUE(parts[1].contains(QStringLiteral("note  app=")));
    EXPECT_TRUE(parts[2].contains(QStringLiteral("note  app=")));
}

TEST(SystemPromptTest, NamesEveryComponent)
{
    const QString prompt = SystemPrompt::text();
    for (const char* component : { "QanvasCountdown", "QanvasStopwatch", "QanvasStat",
                                   "QanvasBadge", "QanvasTabs", "QanvasProgress", "QanvasStepper",
                                   "QanvasPanel", "emptyText", "QanvasChart", "QanvasDatePicker" })
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
    EXPECT_TRUE(prompt.contains(
        QStringLiteral("is kept as one of\nthe words primary|success|warning|danger")));
    EXPECT_TRUE(prompt.contains(QStringLiteral(
        "QanvasSelect { options: [\"primary\", \"success\", \"warning\", \"danger\"] }")));
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
    EXPECT_TRUE(prompt.contains(
        QStringLiteral("QanvasSelect{options value placeholder onSelected(value)}")));
    EXPECT_TRUE(prompt.contains(QStringLiteral("an empty or unknown value shows placeholder")));
}

TEST(SystemPromptTest, TheDatePickerValueIsAnIsoString)
{
    const QString prompt = SystemPrompt::text();
    EXPECT_TRUE(
        prompt.contains(QStringLiteral("QanvasDatePicker{value placeholder onPicked(value)}")));
    EXPECT_TRUE(prompt.contains(QStringLiteral("value is an ISO string \"2026-11-15\"")));
    EXPECT_TRUE(prompt.contains(QStringLiteral("\"\" for no date, which\n    shows placeholder")));
}

TEST(SystemPromptTest, TheStepperSaysNumbersCarryNoThousandsSeparator)
{
    EXPECT_TRUE(SystemPrompt::text().contains(QStringLiteral("it reads 2026, never 2,026")));
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
    for (const char* word : { "QanvasButton{text iconName onClicked type enabled}",
                              "enabled: false", "(primary|secondary|danger|success|warning|ghost)",
                              "badgeRole", "typeRole", "badgeFormat", "typeFormat", "\"done\"",
                              "QanvasProgress{value from to type:success|warning|danger}",
                              "type: \"centiseconds\"", "centiseconds", "m:ss.cc" })
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
    EXPECT_TRUE(defined.contains(QStringLiteral("STEPS")));
    EXPECT_TRUE(defined.contains(QStringLiteral("FIX")));
    EXPECT_TRUE(defined.contains(QStringLiteral("DONE")));
    for (const QString& target : targets)
        EXPECT_TRUE(defined.contains(target)) << target.toStdString();
}

TEST(SystemPromptTest, ATaskOfNumberedStepsRoutesToTheStepsState)
{
    const QString route = sectionOf(SystemPrompt::text(), QStringLiteral("## STATE: ROUTE"));

    EXPECT_TRUE(route.contains(QStringLiteral("a task of numbered steps -> STEPS")));
}

TEST(SystemPromptTest, TheStepsStateBuildsOneStepAtATimeAndChecksEachOne)
{
    const QString steps = sectionOf(SystemPrompt::text(), QStringLiteral("## STATE: STEPS"));

    ASSERT_FALSE(steps.isEmpty());
    EXPECT_TRUE(steps.contains(QStringLiteral("do not redo it, merge steps or add one")));
    EXPECT_TRUE(steps.contains(QStringLiteral("One step at a time, in order")));
    EXPECT_TRUE(steps.contains(QStringLiteral("read_log and check_layout")));
    EXPECT_TRUE(steps.contains(QStringLiteral("the check of the step")));
    EXPECT_TRUE(steps.endsWith(QStringLiteral("-> NOTE")));
}

TEST(SystemPromptTest, TheStepsStateDecidesTheTablesOfEveryStepBeforeTheFirst)
{
    const QString steps = sectionOf(SystemPrompt::text(), QStringLiteral("## STATE: STEPS"));

    EXPECT_TRUE(steps.contains(QStringLiteral("list_apps first")));
    EXPECT_TRUE(steps.contains(QStringLiteral("tables of ALL the steps at once")));
    EXPECT_TRUE(steps.contains(QStringLiteral("shows step 1 only")));
    EXPECT_TRUE(steps.contains(QStringLiteral("app exists INSPECT first, once")));
}

TEST(SystemPromptTest, ACheckThatNeedsATapIsNeverCalledVerified)
{
    const QString steps = sectionOf(SystemPrompt::text(), QStringLiteral("## STATE: STEPS"));

    EXPECT_TRUE(steps.contains(QStringLiteral("A check is verified when the log and the layout "
                                              "are clean")));
    EXPECT_TRUE(steps.contains(QStringLiteral("needs a tap or typed text is NOT verified")));
    EXPECT_TRUE(steps.contains(QStringLiteral("it is done-unverified")));
}

TEST(SystemPromptTest, AStepThatCouldNotBeRepairedIsNotDoneAndStoppingEarlyIsNormal)
{
    const QString steps = sectionOf(SystemPrompt::text(), QStringLiteral("## STATE: STEPS"));

    EXPECT_TRUE(steps.contains(QStringLiteral("A step that FIX could not repair is not done: stop "
                                              "at it")));
    EXPECT_TRUE(steps.contains(QStringLiteral("Leaving steps undone is normal")));
    EXPECT_TRUE(steps.contains(QStringLiteral("reporting as done a step that is not is not")));
    EXPECT_TRUE(steps.contains(QStringLiteral("Stop nowhere else: go on to the next step as long "
                                              "as this one is done")));
}

TEST(SystemPromptTest, TheReportOfATaskWithStepsHasOneLinePerStep)
{
    const QString done = sectionOf(SystemPrompt::text(), QStringLiteral("## STATE: DONE"));

    EXPECT_TRUE(done.contains(QStringLiteral("answered instead with one line per step, numbered "
                                             "as in the task")));
    EXPECT_TRUE(done.contains(QStringLiteral("1 done+verified")));
    EXPECT_TRUE(done.contains(QStringLiteral("2 done-unverified: not tapped")));
    EXPECT_TRUE(done.contains(QStringLiteral("3 not done: ")));
    EXPECT_TRUE(
        done.contains(QStringLiteral("one you never reached is \"not done: not started\"")));
    EXPECT_TRUE(done.contains(QStringLiteral("Report only the steps the task lists, numbered as "
                                             "there: a step it does not list gets no line")));
}

TEST(SystemPromptTest, ThirdExampleBuildsTwoStepsOneAfterTheOtherAndReportsBoth)
{
    const QString example
        = sectionOf(SystemPrompt::text(), QStringLiteral("## EXAMPLE 3 — a task with steps"));

    ASSERT_FALSE(example.isEmpty());
    EXPECT_TRUE(example.contains(QStringLiteral("INSPECT once")));
    EXPECT_TRUE(example.contains(QStringLiteral("Step 1:")));
    EXPECT_TRUE(example.contains(QStringLiteral("Step 2:")));
    EXPECT_LT(example.indexOf(QStringLiteral("Step 1:")),
              example.indexOf(QStringLiteral("Step 2:")));
    EXPECT_TRUE(example.contains(QStringLiteral("step 1 is done-unverified")));
    EXPECT_TRUE(example.contains(QStringLiteral("step 2 is done+verified")));
    EXPECT_TRUE(example.contains(QStringLiteral("DONE: \"1 done-unverified: not tapped")));
    EXPECT_TRUE(example.contains(QStringLiteral("2 done+verified")));
}

TEST(SystemPromptTest, TheMainPromptLoadsFromTheResource)
{
    EXPECT_FALSE(SystemPrompt::mainText().isEmpty());
    EXPECT_NE(SystemPrompt::mainText(), SystemPrompt::text());
}

TEST(SystemPromptTest, TheMainPromptKeepsWhatQanvasIsAndWhoDoesTheBuilding)
{
    const QString main = SystemPrompt::mainText().simplified();

    EXPECT_TRUE(main.startsWith(QStringLiteral("Qanvas is a runtime that runs small QML apps")));
    EXPECT_TRUE(main.contains(QStringLiteral("goes to the builder agent")));
    EXPECT_TRUE(main.contains(QStringLiteral("you cannot touch apps yourself")));
    EXPECT_TRUE(main.contains(QStringLiteral("including each task, in the language the user "
                                             "writes in")));
    EXPECT_TRUE(main.contains(QStringLiteral("Answer questions about Qanvas and plain "
                                             "conversation yourself")));
}

TEST(SystemPromptTest, TheMainPromptWalksAskPlanDelegateReportInThatOrder)
{
    const QString main = SystemPrompt::mainText();
    qsizetype last = -1;

    for (const char* stage : { "\nASK\n", "\nPLAN\n", "\nDELEGATE\n", "\nREPORT\n" })
    {
        const qsizetype at = main.indexOf(QLatin1String(stage));
        EXPECT_GT(at, last) << stage;
        last = at;
    }
    EXPECT_TRUE(main.contains(QStringLiteral("ASK, PLAN, DELEGATE, REPORT")));
}

TEST(SystemPromptTest, TheMainAgentAsksOnlyWhenTheAnswerChangesWhatGetsBuilt)
{
    const QString ask = mainStage(QStringLiteral("ASK"));

    ASSERT_FALSE(ask.isEmpty());
    EXPECT_TRUE(ask.contains(QStringLiteral("Ask only when the answer changes what gets built")));
    EXPECT_TRUE(ask.contains(QStringLiteral("what the app keeps")));
    EXPECT_TRUE(ask.contains(QStringLiteral("a unit or currency")));
    EXPECT_TRUE(ask.contains(QStringLiteral("Looks, names, colours and the order of things are "
                                            "not essential")));
    EXPECT_TRUE(ask.contains(QStringLiteral("When nothing essential is unclear there is no ASK")));
}

TEST(SystemPromptTest, TheQuestionsComeInOneMessageEachWithADefaultAndThenItWaits)
{
    const QString ask = mainStage(QStringLiteral("ASK"));

    EXPECT_TRUE(ask.contains(QStringLiteral("ONE message of at most four short numbered "
                                            "questions")));
    EXPECT_TRUE(ask.contains(QStringLiteral("each with your proposed default in brackets")));
    EXPECT_TRUE(ask.contains(QStringLiteral("stop there: no plan, no delegation")));
    EXPECT_TRUE(ask.contains(QStringLiteral("\"whatever\" or \"as you like\" means the "
                                            "defaults")));
    EXPECT_TRUE(ask.contains(QStringLiteral("1. Currency? (zł)")));
}

TEST(SystemPromptTest, ThePlanListsFunctionsEachWithAOneLineCheckAndIsNeverAFile)
{
    const QString plan = mainStage(QStringLiteral("PLAN"));

    ASSERT_FALSE(plan.isEmpty());
    EXPECT_TRUE(plan.contains(QStringLiteral("ordered list of small steps")));
    EXPECT_TRUE(plan.contains(QStringLiteral("ONE function the user can see")));
    EXPECT_TRUE(plan.contains(QStringLiteral("one-line check: what to put in, what to tap, what "
                                             "must show")));
    EXPECT_TRUE(plan.contains(QStringLiteral("Describe functions, never files or code")));
    EXPECT_TRUE(plan.contains(QStringLiteral("it is never written to a file")));
    EXPECT_EQ(plan.count(QStringLiteral("— check:")), 3);
}

TEST(SystemPromptTest, ALaterChangeGetsANewShortPlanAndEarlierPlansAreNotKept)
{
    const QString plan = mainStage(QStringLiteral("PLAN"));

    EXPECT_TRUE(plan.contains(QStringLiteral("A change to an existing app gets a new short "
                                             "plan")));
    EXPECT_TRUE(plan.contains(QStringLiteral("what the app does now and what the user asks")));
    EXPECT_TRUE(plan.contains(QStringLiteral("earlier plans are not kept")));
    EXPECT_TRUE(plan.contains(QStringLiteral("A small change is a plan of one step")));
}

TEST(SystemPromptTest, TheWholePlanGoesToTheBuilderInOneTaskThatStandsOnItsOwn)
{
    const QString delegate = mainStage(QStringLiteral("DELEGATE"));

    ASSERT_FALSE(delegate.isEmpty());
    EXPECT_TRUE(delegate.contains(QStringLiteral("the whole plan in ONE task")));
    EXPECT_TRUE(delegate.contains(QStringLiteral("does not see this conversation")));
    EXPECT_TRUE(delegate.contains(QStringLiteral("the app id, or \"new app\"")));
    EXPECT_TRUE(delegate.contains(QStringLiteral("the answers and defaults from ASK")));
    EXPECT_TRUE(delegate.contains(QStringLiteral("the steps, numbered, each with its check")));
}

TEST(SystemPromptTest, StepsNotDoneAreDelegatedAgainWithOnlyThoseSteps)
{
    const QString report = mainStage(QStringLiteral("REPORT"));

    ASSERT_FALSE(report.isEmpty());
    EXPECT_TRUE(report.contains(QStringLiteral("It may stop before the end of the list.")));
    EXPECT_TRUE(report.contains(QStringLiteral("delegate again with ONLY those steps")));
    EXPECT_TRUE(report.contains(QStringLiteral("earlier steps are already in the app")));
    EXPECT_TRUE(report.contains(QStringLiteral("A delegation that failed means every step in it "
                                               "is not done")));
}

TEST(SystemPromptTest, AStepNotDoneAfterTwoDelegationsIsHandedBackToTheUser)
{
    const QString report = mainStage(QStringLiteral("REPORT"));

    EXPECT_TRUE(report.contains(QStringLiteral("still not done after two delegations that "
                                               "contained it: stop, tell the user which step "
                                               "and why")));
    EXPECT_TRUE(report.contains(QStringLiteral("do not delegate it again")));
    EXPECT_TRUE(report.contains(QStringLiteral("too_many_delegations: tell the user which steps "
                                               "are missing")));
}

TEST(SystemPromptTest, AnUnverifiedStepIsNotDelegatedAgainButHandedToTheUserWithItsCheck)
{
    const QString report = mainStage(QStringLiteral("REPORT"));

    EXPECT_TRUE(report.contains(QStringLiteral("done-unverified is not delegated again")));
    EXPECT_TRUE(report.contains(QStringLiteral("with its check, so they can try it")));
    EXPECT_TRUE(report.contains(QStringLiteral("Finish with a reply that lists the steps, each "
                                               "with what came of it")));
}

TEST(SystemPromptTest, TheStatusOfAStepComesFromTheDelegationThatContainedItAndIsNeverUpgraded)
{
    const QString report = mainStage(QStringLiteral("REPORT"));

    EXPECT_TRUE(report.contains(QStringLiteral("the status of a step is the one the builder gave "
                                               "in the delegation that contained it"),
                                Qt::CaseInsensitive));
    EXPECT_TRUE(report.contains(QStringLiteral("a line about a step that delegation did not "
                                               "contain is ignored")));
    EXPECT_TRUE(report.contains(QStringLiteral("no later delegation upgrades a status")));
}

TEST(SystemPromptTest, BothPromptsUseTheSameWordsForTheStateOfAStep)
{
    const QString builder = SystemPrompt::text();
    const QString main = SystemPrompt::mainText();

    for (const char* status : { "done+verified", "done-unverified", "not done" })
    {
        EXPECT_TRUE(builder.contains(QLatin1String(status))) << status;
        EXPECT_TRUE(main.contains(QLatin1String(status))) << status;
    }
}

TEST(SystemPromptTest, TheMainPromptDoesNotTeachTheBuilderStates)
{
    const QString main = SystemPrompt::mainText();

    EXPECT_FALSE(main.contains(QStringLiteral("STATE:")));
    EXPECT_FALSE(main.contains(QStringLiteral("edit_file")));
    EXPECT_FALSE(SystemPrompt::text().contains(QStringLiteral("\nASK\n")));
}
