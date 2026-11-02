#include <gtest/gtest.h>

#include <QFile>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "qmlutils/coloredsvgprovider.h"
#include "runtime/host/applayout.h"

namespace
{
const char* kImports = "import QtQuick\nimport QtQuick.Layouts\n";

class AppLayoutTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/Qanvas/Components/Theme.qml")),
                                 "Themed.Components", 1, 0, "Theme");
        qmlRegisterType<ColoredSvgProvider>("Themed.Components", 1, 0, "ColoredSvgProvider");
    }

    QString show(const QString& body)
    {
        const QString path = dir.path() + QStringLiteral("/Main.qml");
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(QByteArray(kImports) + body.toUtf8());
        file.close();

        view.engine()->addImportPath(QStringLiteral(QML_IMPORT_ROOT));
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.resize(200, 300);
        view.setSource(QUrl::fromLocalFile(path));
        EXPECT_EQ(view.status(), QQuickView::Ready);
        view.show();
        QTest::qWaitForWindowExposed(&view);
        QTest::qWait(50);
        return AppLayout::report(view.rootObject(), dir.path());
    }

    QTemporaryDir dir;
    QQuickView view;
};
}

TEST_F(AppLayoutTest, ContentThatFitsReportsNothing)
{
    const QString report = show(
        QStringLiteral("Item { ColumnLayout { anchors.fill: parent; Text { text: \"hi\" } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, ContentTallerThanThePageIsReported)
{
    const QString report = show(QStringLiteral(
        "Item { ColumnLayout { anchors.fill: parent; Repeater { model: 5; Rectangle { "
        "Layout.preferredHeight: 100; Layout.fillWidth: true } } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("200x300"))) << report.toStdString();
    EXPECT_TRUE(report.contains(QStringLiteral("ColumnLayout (Main.qml) needs 520px of height but "
                                               "has 300px")))
        << report.toStdString();
}

TEST_F(AppLayoutTest, ContentInsideAScrollableFlickableIsFine)
{
    const QString report = show(QStringLiteral(
        "Item { Flickable { anchors.fill: parent; contentHeight: column.implicitHeight; "
        "ColumnLayout { id: column; width: parent.width; Repeater { model: 5; Rectangle { "
        "Layout.preferredHeight: 100; Layout.fillWidth: true } } } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, AnItemStickingOutOnTheRightIsNamed)
{
    const QString report = show(
        QStringLiteral("Item { Text { text: \"a rather long line of text that will not fit\" } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("Text \"a rather long line of t")))
        << report.toStdString();
    EXPECT_TRUE(report.contains(QStringLiteral("sticks out of the screen on the right")))
        << report.toStdString();
    EXPECT_TRUE(report.contains(QStringLiteral("(Main.qml)"))) << report.toStdString();
}

TEST_F(AppLayoutTest, ARowThatDoesNotFitIsReportedOnce)
{
    const QString report = show(
        QStringLiteral("Item { RowLayout { width: parent.width; Repeater { model: 3; Rectangle { "
                       "Layout.preferredWidth: 100; Layout.preferredHeight: 40 } } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("needs"))) << report.toStdString();
    EXPECT_EQ(report.count(QStringLiteral("\n- ")), 1) << report.toStdString();
}

TEST_F(AppLayoutTest, AWrappedTextIsNotTooWide)
{
    const QString report = show(QStringLiteral(
        "Item { ColumnLayout { anchors.fill: parent; Text { Layout.fillWidth: true; wrapMode: "
        "Text.WordWrap; text: \"a rather long line of text that will not fit on one line\" } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, AnItemWithoutHeightIsReported)
{
    const QString report = show(QStringLiteral(
        "Item { ColumnLayout { anchors.fill: parent; ListView { Layout.fillWidth: true; "
        "model: 3; delegate: Text { text: \"row\" } } Item { Layout.fillHeight: true } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("ListView"))) << report.toStdString();
    EXPECT_TRUE(report.contains(QStringLiteral("has no height"))) << report.toStdString();
}

TEST_F(AppLayoutTest, AnItemWhoseContentLivesOnTheOverlayIsNotReported)
{
    const QString report = show(QStringLiteral(
        "import QtQuick.Controls\n"
        "Item { id: page; ColumnLayout { anchors.fill: parent; Text { text: \"hi\" } Item { "
        "Rectangle { parent: Overlay.overlay ? Overlay.overlay : page; width: 50; height: 20 } } "
        "} }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, AnItemHostingAPopupIsNotReported)
{
    const QString report = show(QStringLiteral(
        "import QtQuick.Controls\n"
        "Item { ColumnLayout { anchors.fill: parent; Text { text: \"hi\" } Item { Dialog { "
        "title: \"t\" } } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, AnItemThatDrawsNothingAndHoldsNothingIsNotReported)
{
    const QString report = show(
        QStringLiteral("Item { ColumnLayout { anchors.fill: parent; Text { text: \"hi\" } Item { "
                       "Layout.fillWidth: true } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, AnItemThatDrawsAndHasNoHeightIsStillReported)
{
    const QString report = show(QStringLiteral(
        "Item { ColumnLayout { anchors.fill: parent; Text { text: \"hi\" } Rectangle { color: "
        "\"red\"; Layout.fillWidth: true } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("Rectangle"))) << report.toStdString();
    EXPECT_TRUE(report.contains(QStringLiteral("has no height"))) << report.toStdString();
}

TEST_F(AppLayoutTest, TheComponentsThatDrawNothingOrOnlyALineAreNotReported)
{
    const QString report = show(QStringLiteral(
        "import Qanvas.Components\n"
        "Item { QanvasColumn { anchors.fill: parent; QanvasText { text: \"hi\" } QanvasSeparator "
        "{ } QanvasStopwatch { running: true } QanvasCountdown { seconds: 60 } QanvasToast { } "
        "QanvasSpacer { Layout.fillHeight: true } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, ASeparatorWithoutThicknessIsReportedLikeAnyDrawnItem)
{
    const QString report = show(QStringLiteral(
        "import Qanvas.Components\n"
        "Item { QanvasColumn { anchors.fill: parent; QanvasText { text: \"hi\" } QanvasSeparator "
        "{ Layout.preferredHeight: 0 } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("QanvasSeparator"))) << report.toStdString();
    EXPECT_TRUE(report.contains(QStringLiteral("has no height"))) << report.toStdString();
}

TEST_F(AppLayoutTest, HiddenItemsAreSkipped)
{
    const QString report = show(QStringLiteral(
        "Item { Text { visible: false; text: \"a rather long line of text that will not fit\" } "
        "}"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, ATextCutOffInsideAComponentIsReportedUnderTheOwner)
{
    const QString report = show(QStringLiteral(
        "import Qanvas.Components\n"
        "Item { QanvasColumn { anchors.fill: parent; QanvasListItem { Layout.fillWidth: false; "
        "Layout.preferredWidth: 90; title: \"Kup mleko i chleb w sklepie\" } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("QanvasListItem \"Kup mleko i chleb w skl…\" "
                                               "(Main.qml): the text \"Kup mleko i chleb w "
                                               "skl…\" does not fit: it needs")))
        << report.toStdString();
    EXPECT_EQ(report.count(QStringLiteral("\n- ")), 1) << report.toStdString();
}

TEST_F(AppLayoutTest, AnElidedTextTheAppWroteItselfIsReported)
{
    const QString report = show(QStringLiteral(
        "Item { Text { width: 60; text: \"a rather long line\"; elide: Text.ElideRight } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("Text \"a rather long line\" (Main.qml): the text "
                                               "\"a rather long line\" does not fit")))
        << report.toStdString();
}

TEST_F(AppLayoutTest, ComponentsWhoseTextFitsAreNotReported)
{
    const QString report = show(QStringLiteral(
        "import Qanvas.Components\n"
        "Item { QanvasColumn { anchors.fill: parent; QanvasListItem { title: \"Mleko\"; "
        "subtitle: \"Zrobione\"; checkable: true; removable: true } QanvasStat { label: \"Suma\"; "
        "value: \"12,50 zł\" } QanvasBadge { text: \"ok\" } QanvasButton { text: \"Dodaj\" } "
        "QanvasText { text: \"Zostało do zrobienia: 3\" } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, SiblingsStackedAtOnePositionAreReported)
{
    const QString report = show(QStringLiteral("Item { Repeater { model: 3; Row { Text { text: "
                                               "\"row \" + index + \" with text\" } } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("Row (Main.qml) overlaps Row (Main.qml) by")))
        << report.toStdString();
    EXPECT_TRUE(report.contains(QStringLiteral("; 1 more overlap in the same place")))
        << report.toStdString();
    EXPECT_EQ(report.count(QStringLiteral("\n- ")), 1) << report.toStdString();
}

TEST_F(AppLayoutTest, ListItemsOfARepeaterInsideAFlickableAreReportedAsStacked)
{
    const QString report = show(QStringLiteral(
        "import Qanvas.Components\n"
        "Item { Flickable { anchors.fill: parent; Repeater { model: 3; QanvasListItem { title: "
        "\"Kup mleko\" + index } } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("QanvasListItem \"Kup mleko0\" (Main.qml) "
                                               "overlaps QanvasListItem \"Kup mleko1\" "
                                               "(Main.qml) by")))
        << report.toStdString();
}

TEST_F(AppLayoutTest, TwoTextsThatOverlapPartlyAreReportedWithTheDepth)
{
    const QString report = show(QStringLiteral(
        "Item { Text { text: \"first heading line\" } Text { x: 10; y: 5; text: \"second\" } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("Text \"first heading line\" (Main.qml) overlaps "
                                               "Text \"second\" (Main.qml) by ")))
        << report.toStdString();
    EXPECT_TRUE(report.contains(QStringLiteral("px of height"))) << report.toStdString();
}

TEST_F(AppLayoutTest, ATextDrawnInsideAnotherTextBlockIsReported)
{
    const QString report = show(QStringLiteral(
        "Item { Column { Text { text: \"a long heading line\" } Text { text: \"second\" } } Text { "
        "text: \"x\" } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("overlaps"))) << report.toStdString();
}

TEST_F(AppLayoutTest, ABackgroundAndAnOverlayAreNotOverlaps)
{
    const QString report = show(QStringLiteral(
        "Item { Rectangle { anchors.fill: parent; color: \"white\" } Text { anchors.centerIn: "
        "parent; text: \"hi\" } Rectangle { anchors.fill: parent; color: \"#80000000\" } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, ContentOfALayoutDoesNotOverlap)
{
    const QString report = show(QStringLiteral(
        "import Qanvas.Components\n"
        "Item { QanvasColumn { anchors.fill: parent; QanvasRow { QanvasText { text: \"one\" } "
        "QanvasText { text: \"two\" } } QanvasProgress { value: 3; from: 0; to: 10 } QanvasText { "
        "text: \"three\" } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, AnItemPastAClippingParentIsReportedOnceWithTheSide)
{
    const QString report = show(QStringLiteral(
        "Item { Item { width: 100; height: 100; clip: true; Rectangle { x: 80; y: 10; width: 50; "
        "height: 20; color: \"red\"; Rectangle { anchors.fill: parent; color: \"blue\" } } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("Rectangle (Main.qml) is cut off by Item "
                                               "(Main.qml) on the right by 30px")))
        << report.toStdString();
    EXPECT_EQ(report.count(QStringLiteral("\n- ")), 1) << report.toStdString();
}

TEST_F(AppLayoutTest, ContentOfAComponentPastAClippingParentIsReportedUnderTheOwner)
{
    const QString report = show(QStringLiteral(
        "import Qanvas.Components\n"
        "Item { Item { width: 90; height: 100; clip: true; QanvasButton { text: \"Dodaj\" } } "
        "}"));

    EXPECT_TRUE(report.contains(QStringLiteral("QanvasButton \"Dodaj\" (Main.qml) is cut off by "
                                               "Item (Main.qml) on the right by")))
        << report.toStdString();
}

TEST_F(AppLayoutTest, ContentPastTheEndOfAnInteractiveFlickableIsNotCutOff)
{
    const QString report = show(QStringLiteral(
        "Item { Flickable { width: 100; height: 100; contentHeight: 300; clip: true; Rectangle { "
        "y: 80; width: 50; height: 60; color: \"red\" } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}

TEST_F(AppLayoutTest, ContentPastTheEndOfAFlickableThatCannotScrollIsCutOff)
{
    const QString report = show(QStringLiteral(
        "Item { Flickable { width: 100; height: 100; interactive: false; contentHeight: 300; clip: "
        "true; Rectangle { y: 80; width: 50; height: 60; color: \"red\" } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("is cut off by Flickable (Main.qml) at the bottom "
                                               "by 40px")))
        << report.toStdString();
}

TEST_F(AppLayoutTest, ContentPastTheSideOfAVerticalScrollIsCutOff)
{
    const QString report = show(QStringLiteral(
        "Item { Flickable { width: 100; height: 100; contentWidth: width; contentHeight: 300; "
        "clip: true; Rectangle { x: 60; y: 200; width: 50; height: 20; color: \"red\" } } }"));

    EXPECT_TRUE(report.contains(QStringLiteral("on the right by 10px"))) << report.toStdString();
    EXPECT_FALSE(report.contains(QStringLiteral("at the bottom"))) << report.toStdString();
}

TEST_F(AppLayoutTest, ALongColumnInAScrollIsNotReported)
{
    const QString report = show(QStringLiteral(
        "import Qanvas.Components\n"
        "Item { QanvasScroll { anchors.fill: parent; Repeater { model: 20; QanvasListItem { "
        "title: \"Wiersz \" + index } } } }"));

    EXPECT_TRUE(report.isEmpty()) << report.toStdString();
}
