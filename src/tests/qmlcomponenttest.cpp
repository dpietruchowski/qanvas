#include "qmlharness.h"

class QmlComponentTest : public QmlHarness
{
protected:
    QQuickItem* row(QQuickItem* parent, const QString& name) const
    {
        for (QQuickItem* child : parent->childItems())
        {
            if (child->objectName() == name)
                return child;
            if (QQuickItem* found = row(child, name))
                return found;
        }
        return nullptr;
    }
};

TEST_F(QmlComponentTest, AStepperShowsAnIntegerWithoutGrouping)
{
    QQuickItem* item = show(QStringLiteral(
        "QanvasStepper { from: 1900; to: 2100; value: 2026; property string shown: contentItem.text }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("2026"));
}

TEST_F(QmlComponentTest, ASelectWithAnEmptyValueShowsItsPlaceholderAndNoOption)
{
    QQuickItem* item = show(QStringLiteral("QanvasSelect { options: [\"Food\", \"Rent\"]; value: \"\"; "
                                           "placeholder: \"Category\"; "
                                           "property string shown: contentItem.text }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("Category"));
    EXPECT_EQ(item->property("currentIndex").toInt(), -1);
    EXPECT_EQ(item->property("value").toString(), QString());
}

TEST_F(QmlComponentTest, ASelectWithAnUnknownValueShowsItsPlaceholder)
{
    QQuickItem* item = show(QStringLiteral("QanvasSelect { options: [\"Food\", \"Rent\"]; value: \"Gone\"; "
                                           "placeholder: \"Category\"; "
                                           "property string shown: contentItem.text }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("Category"));
    EXPECT_EQ(item->property("currentIndex").toInt(), -1);
}

TEST_F(QmlComponentTest, ASelectShowsTheOptionEqualToItsValue)
{
    QQuickItem* item = show(QStringLiteral("QanvasSelect { options: [\"Food\", \"Rent\"]; value: \"Rent\"; "
                                           "placeholder: \"Category\"; "
                                           "property string shown: contentItem.text }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("Rent"));
    EXPECT_EQ(item->property("currentIndex").toInt(), 1);
}

TEST_F(QmlComponentTest, ASelectFollowsAnExternalChangeOfItsValue)
{
    QQuickItem* item = show(QStringLiteral(
        "Item { id: host; property string chosen: \"\"; property alias shown: sel.shown; "
        "QanvasSelect { id: sel; options: [\"Food\", \"Rent\"]; value: host.chosen; "
        "placeholder: \"Category\"; property string shown: contentItem.text } }"));
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("Category"));

    item->setProperty("chosen", QStringLiteral("Food"));
    QTest::qWait(30);
    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("Food"));

    item->setProperty("chosen", QString());
    QTest::qWait(30);
    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("Category"));
}

TEST_F(QmlComponentTest, AButtonOfTypeWarningTakesTheWarningStyle)
{
    QQuickItem* item = show(QStringLiteral(
        "QanvasButton { text: \"Wait\"; type: \"warning small\"; "
        "property color shown: buttonStyle.background; "
        "property color wanted: Theme.button.warning.background; "
        "property color primary: Theme.button.primary.background }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").value<QColor>(), item->property("wanted").value<QColor>());
    EXPECT_NE(item->property("shown").value<QColor>(), item->property("primary").value<QColor>());
}

TEST_F(QmlComponentTest, ABadgeWithoutATypeIsTheNeutralOne)
{
    QQuickItem* item = show(QStringLiteral(
        "QanvasBadge { text: \"Misc\"; type: \"\"; "
        "property color shown: Theme.badgeStyle(type).background; "
        "property color neutral: Theme.badge.neutral.background; "
        "property color primary: Theme.badge.primary.background }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").value<QColor>(), item->property("neutral").value<QColor>());
    EXPECT_NE(item->property("shown").value<QColor>(), item->property("primary").value<QColor>());
}

TEST_F(QmlComponentTest, AChartScalesEveryBarToTheLargestValue)
{
    QQuickItem* chart = show(QStringLiteral(
        "QanvasChart { objectName: \"chart\"; textRole: \"name\"; valueRole: \"sum\"; "
        "model: [{name: \"Food\", sum: 30}, {name: \"Rent\", sum: 120}] }"));
    ASSERT_NE(chart, nullptr);

    ASSERT_EQ(chart->property("count").toInt(), 2);
    QQuickItem* food = row(chart, QStringLiteral("chart_row0"));
    QQuickItem* rent = row(chart, QStringLiteral("chart_row1"));
    ASSERT_NE(food, nullptr);
    ASSERT_NE(rent, nullptr);
    EXPECT_DOUBLE_EQ(food->property("fraction").toDouble(), 0.25);
    EXPECT_DOUBLE_EQ(rent->property("fraction").toDouble(), 1.0);
    EXPECT_EQ(food->property("text").toString(), QStringLiteral("Food"));
    EXPECT_EQ(food->property("valueText").toString(), QStringLiteral("30"));
}

TEST_F(QmlComponentTest, AChartFormatsTheValueTextWithItsHook)
{
    QQuickItem* chart = show(QStringLiteral(
        "QanvasChart { objectName: \"chart\"; textRole: \"name\"; valueRole: \"sum\"; "
        "valueFormat: function (item) { return item.sum.toFixed(2) + \" zl\" } "
        "model: [{name: \"Food\", sum: 30}] }"));
    ASSERT_NE(chart, nullptr);

    QQuickItem* food = row(chart, QStringLiteral("chart_row0"));
    ASSERT_NE(food, nullptr);
    EXPECT_EQ(food->property("valueText").toString(), QStringLiteral("30.00 zl"));
    EXPECT_DOUBLE_EQ(food->property("fraction").toDouble(), 1.0);
}

TEST_F(QmlComponentTest, AChartColoursABarByTheTypeWordOfItsItem)
{
    QQuickItem* chart = show(QStringLiteral(
        "QanvasChart { objectName: \"chart\"; textRole: \"name\"; valueRole: \"sum\"; "
        "typeRole: \"kind\"; "
        "property color danger: Theme.progressColor(\"danger\"); "
        "property color success: Theme.progressColor(\"success\"); "
        "property color plain: Theme.progressColor(\"\"); "
        "model: [{name: \"Rent\", sum: 5, kind: \"danger\"}, "
        "{name: \"Salary\", sum: 4, kind: \"success\"}, {name: \"Misc\", sum: 3}] }"));
    ASSERT_NE(chart, nullptr);

    QQuickItem* rent = row(chart, QStringLiteral("chart_row0"));
    QQuickItem* salary = row(chart, QStringLiteral("chart_row1"));
    QQuickItem* misc = row(chart, QStringLiteral("chart_row2"));
    ASSERT_NE(rent, nullptr);
    ASSERT_NE(salary, nullptr);
    ASSERT_NE(misc, nullptr);
    EXPECT_EQ(rent->property("barColor").value<QColor>(), chart->property("danger").value<QColor>());
    EXPECT_EQ(salary->property("barColor").value<QColor>(), chart->property("success").value<QColor>());
    EXPECT_EQ(misc->property("barColor").value<QColor>(), chart->property("plain").value<QColor>());
    EXPECT_NE(rent->property("barColor").value<QColor>(), misc->property("barColor").value<QColor>());
}

TEST_F(QmlComponentTest, AChartOfZeroesDrawsEmptyBarsAndAnEmptyChartNoRows)
{
    QQuickItem* zeroes = show(QStringLiteral("QanvasChart { objectName: \"chart\"; textRole: \"name\"; "
                                             "valueRole: \"sum\"; model: [{name: \"Food\", sum: 0}] }"));
    ASSERT_NE(zeroes, nullptr);
    QQuickItem* food = row(zeroes, QStringLiteral("chart_row0"));
    ASSERT_NE(food, nullptr);
    EXPECT_DOUBLE_EQ(food->property("fraction").toDouble(), 0.0);

    QQuickItem* empty = show(QStringLiteral("QanvasChart { valueRole: \"sum\"; model: [] }"));
    ASSERT_NE(empty, nullptr);
    EXPECT_EQ(empty->property("count").toInt(), 0);
}

TEST_F(QmlComponentTest, ACardInAGridIsTallEnoughForItsContentAndItsMargins)
{
    QQuickItem* grid = show(QStringLiteral(
        "QanvasGrid { id: grid; width: 380; property real content: Theme.card.sizeSmall - Theme.padding.medium; "
        "property real margins: Theme.padding.medium; "
        "QanvasCard { objectName: \"card\"; Rectangle { Layout.fillWidth: true; "
        "Layout.preferredHeight: grid.content } } "
        "QanvasCard { Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 10 } } }"));
    ASSERT_NE(grid, nullptr);

    QQuickItem* card = row(grid, QStringLiteral("card"));
    ASSERT_NE(card, nullptr);
    EXPECT_GE(card->height(), grid->property("content").toDouble() + 2 * grid->property("margins").toDouble());
}
