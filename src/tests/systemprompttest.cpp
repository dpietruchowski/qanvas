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
    for (const char* tool :
         { "create_app", "write_file", "edit_file", "delete_file", "delete_app", "list_files",
           "read_file", "search_files", "describe_app", "read_log", "grant_access" })
        EXPECT_TRUE(prompt.contains(QLatin1String(tool))) << tool;
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
