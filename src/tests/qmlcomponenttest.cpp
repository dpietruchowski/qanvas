#include <QDate>
#include <QFile>
#include <QLocale>
#include <QSet>
#include <QUrl>

#include "qmlharness.h"
#include "runtime/chat/systemprompt.h"

namespace
{
QStringList iconNamesInThePrompt()
{
    const QString prompt = SystemPrompt::text();
    const QString marker = QStringLiteral("iconName picks from:");
    const qsizetype start = prompt.indexOf(marker);
    if (start < 0)
        return {};
    const qsizetype from = start + marker.size();
    const qsizetype end = prompt.indexOf(QChar(0x2014), from);
    return prompt.mid(from, end - from).simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
}
}

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
    QQuickItem* item = show(QStringLiteral("QanvasStepper { from: 1900; to: 2100; value: 2026; "
                                           "property string shown: contentItem.text }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("2026"));
}

TEST_F(QmlComponentTest, ASelectWithAnEmptyValueShowsItsPlaceholderAndNoOption)
{
    QQuickItem* item
        = show(QStringLiteral("QanvasSelect { options: [\"Food\", \"Rent\"]; value: \"\"; "
                              "placeholder: \"Category\"; "
                              "property string shown: contentItem.text }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("Category"));
    EXPECT_EQ(item->property("currentIndex").toInt(), -1);
    EXPECT_EQ(item->property("value").toString(), QString());
}

TEST_F(QmlComponentTest, ASelectWithAnUnknownValueShowsItsPlaceholder)
{
    QQuickItem* item
        = show(QStringLiteral("QanvasSelect { options: [\"Food\", \"Rent\"]; value: \"Gone\"; "
                              "placeholder: \"Category\"; "
                              "property string shown: contentItem.text }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").toString(), QStringLiteral("Category"));
    EXPECT_EQ(item->property("currentIndex").toInt(), -1);
}

TEST_F(QmlComponentTest, ASelectShowsTheOptionEqualToItsValue)
{
    QQuickItem* item
        = show(QStringLiteral("QanvasSelect { options: [\"Food\", \"Rent\"]; value: \"Rent\"; "
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

TEST_F(QmlComponentTest, TabsWithoutAValueStartOnTheFirstOption)
{
    QQuickItem* item = show(QStringLiteral("QanvasTabs { options: [\"Dziś\", \"Historia\"] }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("value").toString(), QStringLiteral("Dziś"));
    EXPECT_EQ(item->property("currentIndex").toInt(), 0);
}

TEST_F(QmlComponentTest, TabsSelectTheOptionEqualToTheirValue)
{
    QQuickItem* item = show(QStringLiteral(
        "QanvasTabs { options: [\"Dziś\", \"Historia\", \"Statystyki\"]; value: \"Historia\" }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("currentIndex").toInt(), 1);
}

TEST_F(QmlComponentTest, TabsWithAValueThatMatchesNoOptionSelectNone)
{
    QQuickItem* item = show(
        QStringLiteral("QanvasTabs { options: [\"Dziś\", \"Historia\"]; value: \"Edycja\" }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("currentIndex").toInt(), -1);
}

TEST_F(QmlComponentTest, AButtonOfTypeWarningTakesTheWarningStyle)
{
    QQuickItem* item
        = show(QStringLiteral("QanvasButton { text: \"Wait\"; type: \"warning small\"; "
                              "property color shown: buttonStyle.background; "
                              "property color wanted: Theme.button.warning.background; "
                              "property color primary: Theme.button.primary.background }"));
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->property("shown").value<QColor>(), item->property("wanted").value<QColor>());
    EXPECT_NE(item->property("shown").value<QColor>(), item->property("primary").value<QColor>());
}

TEST_F(QmlComponentTest, ABadgeWithoutATypeIsTheNeutralOne)
{
    QQuickItem* item
        = show(QStringLiteral("QanvasBadge { text: \"Misc\"; type: \"\"; "
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
    EXPECT_EQ(rent->property("barColor").value<QColor>(),
              chart->property("danger").value<QColor>());
    EXPECT_EQ(salary->property("barColor").value<QColor>(),
              chart->property("success").value<QColor>());
    EXPECT_EQ(misc->property("barColor").value<QColor>(), chart->property("plain").value<QColor>());
    EXPECT_NE(rent->property("barColor").value<QColor>(),
              misc->property("barColor").value<QColor>());
}

TEST_F(QmlComponentTest, AChartOfZeroesDrawsEmptyBarsAndAnEmptyChartNoRows)
{
    QQuickItem* zeroes
        = show(QStringLiteral("QanvasChart { objectName: \"chart\"; textRole: \"name\"; "
                              "valueRole: \"sum\"; model: [{name: \"Food\", sum: 0}] }"));
    ASSERT_NE(zeroes, nullptr);
    QQuickItem* food = row(zeroes, QStringLiteral("chart_row0"));
    ASSERT_NE(food, nullptr);
    EXPECT_DOUBLE_EQ(food->property("fraction").toDouble(), 0.0);

    QQuickItem* empty = show(QStringLiteral("QanvasChart { valueRole: \"sum\"; model: [] }"));
    ASSERT_NE(empty, nullptr);
    EXPECT_EQ(empty->property("count").toInt(), 0);
}

TEST_F(QmlComponentTest, ADatePickerShowsItsIsoValueOrThePlaceholder)
{
    QQuickItem* picker
        = show(QStringLiteral("QanvasDatePicker { value: \"2026-11-02\"; placeholder: \"Due\"; "
                              "property string shown: contentItem.children[0].text }"));
    ASSERT_NE(picker, nullptr);
    EXPECT_EQ(picker->property("shown").toString(), QStringLiteral("2026-11-02"));

    for (const char* value : { "", "2026-02-30", "02/11/2026", "tomorrow" })
    {
        picker->setProperty("value", QString::fromLatin1(value));
        EXPECT_EQ(picker->property("shown").toString(), QStringLiteral("Due")) << value;
    }
}

TEST_F(QmlComponentTest, ADatePickerOpensOnTheMonthOfItsValue)
{
    QQuickItem* picker = show(QStringLiteral("QanvasDatePicker { value: \"2026-12-15\" }"));
    ASSERT_NE(picker, nullptr);

    EXPECT_EQ(picker->property("viewYear").toInt(), 2026);
    EXPECT_EQ(picker->property("viewMonth").toInt(), 11);
}

TEST_F(QmlComponentTest, ADatePickerListsSixWholeWeeksStartingOnTheFirstDayOfTheLocale)
{
    QQuickItem* picker = show(QStringLiteral("QanvasDatePicker { value: \"2026-11-02\" }"));
    ASSERT_NE(picker, nullptr);

    const QVariantList days = picker->property("days").toList();
    ASSERT_EQ(days.size(), 42);
    QDate previous;
    int inMonth = 0;
    for (const QVariant& entry : days)
    {
        const QVariantMap day = entry.toMap();
        const QDate date
            = QDate::fromString(day.value(QStringLiteral("iso")).toString(), Qt::ISODate);
        ASSERT_TRUE(date.isValid());
        EXPECT_EQ(day.value(QStringLiteral("day")).toInt(), date.day());
        EXPECT_EQ(day.value(QStringLiteral("inMonth")).toBool(),
                  date.month() == 11 && date.year() == 2026);
        if (previous.isValid())
            EXPECT_EQ(previous.daysTo(date), 1);
        previous = date;
        inMonth += day.value(QStringLiteral("inMonth")).toBool() ? 1 : 0;
    }
    EXPECT_EQ(inMonth, 30);
    const QDate first = QDate::fromString(
        days.first().toMap().value(QStringLiteral("iso")).toString(), Qt::ISODate);
    EXPECT_EQ(first.dayOfWeek(), static_cast<int>(QLocale().firstDayOfWeek()));
    EXPECT_LE(first, QDate(2026, 11, 1));
    EXPECT_LT(first.daysTo(QDate(2026, 11, 1)), 7);
}

TEST_F(QmlComponentTest, ADatePickerStepsMonthsAcrossTheEndOfAYear)
{
    QQuickItem* picker = show(QStringLiteral("QanvasDatePicker { value: \"2026-12-15\" }"));
    ASSERT_NE(picker, nullptr);

    QMetaObject::invokeMethod(picker, "step", Q_ARG(QVariant, 1));
    EXPECT_EQ(picker->property("viewYear").toInt(), 2027);
    EXPECT_EQ(picker->property("viewMonth").toInt(), 0);

    QMetaObject::invokeMethod(picker, "step", Q_ARG(QVariant, -2));
    EXPECT_EQ(picker->property("viewYear").toInt(), 2026);
    EXPECT_EQ(picker->property("viewMonth").toInt(), 10);
}

TEST_F(QmlComponentTest, PickingADayStoresItAsAnIsoStringAndEmitsPicked)
{
    QQuickItem* picker = show(QStringLiteral("QanvasDatePicker { property string last: \"\"; "
                                             "onPicked: function (value) { last = value } }"));
    ASSERT_NE(picker, nullptr);

    QMetaObject::invokeMethod(picker, "pick", Q_ARG(QVariant, QStringLiteral("2026-11-15")));

    EXPECT_EQ(picker->property("value").toString(), QStringLiteral("2026-11-15"));
    EXPECT_EQ(picker->property("last").toString(), QStringLiteral("2026-11-15"));
    EXPECT_TRUE(picker->property("hasValue").toBool());
}

TEST_F(QmlComponentTest, ALongTextWrapsWithinTheWidthOfItsColumn)
{
    QQuickItem* column
        = show(QStringLiteral("ColumnLayout { width: 300; "
                              "QanvasText { objectName: \"short\"; text: \"Short\" } "
                              "QanvasText { objectName: \"long\"; text: \"No limit was exceeded "
                              "this month, so nothing is "
                              "marked red and nothing needs your attention right now\" } }"));
    ASSERT_NE(column, nullptr);

    QQuickItem* shortText = row(column, QStringLiteral("short"));
    QQuickItem* longText = row(column, QStringLiteral("long"));
    ASSERT_NE(shortText, nullptr);
    ASSERT_NE(longText, nullptr);

    EXPECT_LE(longText->width(), column->width());
    EXPECT_GT(longText->property("implicitWidth").toDouble(), column->width());
    EXPECT_GE(longText->height(), 2 * shortText->height());
}

TEST_F(QmlComponentTest, TheDatePickerClosesOnEscape)
{
    QQuickItem* host = show(QStringLiteral(
        "Item { QanvasDatePicker { objectName: \"picker\"; y: 100; value: \"2026-11-15\" } }"));
    ASSERT_NE(host, nullptr);
    QQuickItem* picker = row(host, QStringLiteral("picker"));
    ASSERT_NE(picker, nullptr);
    QObject* popup = picker->findChild<QObject*>(QStringLiteral("picker_popup"));
    ASSERT_NE(popup, nullptr);

    QMetaObject::invokeMethod(picker, "keyActivate");
    QTest::qWait(100);
    ASSERT_TRUE(popup->property("visible").toBool());

    QTest::keyClick(&window, Qt::Key_Escape);
    QTest::qWait(100);
    EXPECT_FALSE(popup->property("visible").toBool());
}

TEST_F(QmlComponentTest, TheDatePickerOpensBelowItsFieldAndAboveItWhenThereIsNoRoomBelow)
{
    QQuickItem* host = show(QStringLiteral(
        "Item { QanvasDatePicker { objectName: \"high\"; y: 20; value: \"2026-11-15\" } "
        "QanvasDatePicker { objectName: \"low\"; y: 740; value: \"2026-11-15\" } }"));
    ASSERT_NE(host, nullptr);

    for (const auto& [name, below] :
         { std::pair { QStringLiteral("high"), true }, std::pair { QStringLiteral("low"), false } })
    {
        QQuickItem* picker = row(host, name);
        ASSERT_NE(picker, nullptr) << name.toStdString();
        QObject* popup = picker->findChild<QObject*>(name + QStringLiteral("_popup"));
        ASSERT_NE(popup, nullptr) << name.toStdString();

        QMetaObject::invokeMethod(picker, "keyActivate");
        QTest::qWait(100);
        ASSERT_TRUE(popup->property("visible").toBool()) << name.toStdString();

        const qreal popupY = popup->property("y").toReal();
        if (below)
            EXPECT_GE(popupY, picker->height()) << name.toStdString();
        else
            EXPECT_LE(popupY + popup->property("height").toReal(), 0) << name.toStdString();

        QMetaObject::invokeMethod(popup, "close");
        QTest::qWait(100);
    }
}

TEST_F(QmlComponentTest, ACardInAGridIsTallEnoughForItsContentAndItsMargins)
{
    QQuickItem* grid = show(QStringLiteral(
        "QanvasGrid { id: grid; width: 380; property real content: Theme.card.sizeSmall - "
        "Theme.padding.medium; "
        "property real margins: Theme.padding.medium; "
        "QanvasCard { objectName: \"card\"; Rectangle { Layout.fillWidth: true; "
        "Layout.preferredHeight: grid.content } } "
        "QanvasCard { Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 10 } } }"));
    ASSERT_NE(grid, nullptr);

    QQuickItem* card = row(grid, QStringLiteral("card"));
    ASSERT_NE(card, nullptr);
    EXPECT_GE(card->height(),
              grid->property("content").toDouble() + 2 * grid->property("margins").toDouble());
}

TEST_F(QmlComponentTest, ThePromptListsExactlyTheIconNamesOfTheTheme)
{
    QQuickItem* item
        = show(QStringLiteral("Item { property list<string> names: Theme.iconNames }"));
    ASSERT_NE(item, nullptr);

    const QStringList names = item->property("names").toStringList();

    EXPECT_FALSE(names.isEmpty());
    EXPECT_EQ(iconNamesInThePrompt(), names);
    EXPECT_EQ(QSet<QString>(names.begin(), names.end()).size(), names.size());
}

TEST_F(QmlComponentTest, EveryIconNameAndAliasResolvesToAnIconFile)
{
    QQuickItem* item = show(
        QStringLiteral("Item { property list<string> names: Theme.iconNames; "
                       "property list<string> aliases: Object.keys(Theme.iconAliases); "
                       "property list<string> nameSources: names.map(name => source(name)); "
                       "property list<string> aliasSources: aliases.map(name => source(name)); "
                       "function source(name) { try { return Theme.iconSource(name) } "
                       "catch (e) { return \"\" } } }"));
    ASSERT_NE(item, nullptr);

    const auto expectFiles = [item](const char* keys, const char* sources)
    {
        const QStringList wanted = item->property(keys).toStringList();
        const QStringList found = item->property(sources).toStringList();
        ASSERT_FALSE(wanted.isEmpty());
        ASSERT_EQ(wanted.size(), found.size());
        for (qsizetype index = 0; index < wanted.size(); ++index)
            EXPECT_TRUE(QFile::exists(QLatin1Char(':') + QUrl(found.at(index)).path()))
                << wanted.at(index).toStdString() << " -> " << found.at(index).toStdString();
    };

    expectFiles("names", "nameSources");
    expectFiles("aliases", "aliasSources");
}

TEST_F(QmlComponentTest, NoIconAliasShadowsAnIconName)
{
    QQuickItem* item = show(QStringLiteral("Item { property list<string> names: Theme.iconNames; "
                                           "property list<string> aliases: "
                                           "Object.keys(Theme.iconAliases) }"));
    ASSERT_NE(item, nullptr);

    const QStringList names = item->property("names").toStringList();
    for (const QString& alias : item->property("aliases").toStringList())
        EXPECT_FALSE(names.contains(alias)) << alias.toStdString();
}
