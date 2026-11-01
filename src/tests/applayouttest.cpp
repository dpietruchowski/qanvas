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
