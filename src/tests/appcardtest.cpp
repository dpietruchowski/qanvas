#include "qmlharness.h"

class AppCardTest : public QmlHarness
{
protected:
    static QQuickItem* named(QQuickItem* parent, const QString& name)
    {
        for (QQuickItem* child : parent->childItems())
        {
            if (child->objectName() == name)
                return child;
            if (QQuickItem* found = named(child, name))
                return found;
        }
        return nullptr;
    }

    static QRectF rectIn(QQuickItem* item, QQuickItem* space)
    {
        return item->mapRectToItem(space, QRectF(0, 0, item->width(), item->height()));
    }

    QQuickItem* tile(const QString& title)
    {
        return show(
            QStringLiteral("import Qanvas\n"
                           "import Themed.Components\n"
                           "AppCard { width: Theme.appList.tileMinWidth - Theme.appList.tileGap; "
                           "height: Theme.appList.tileHeight - Theme.appList.tileGap; "
                           "title: \"%1\"; icon: \"star\" }")
                .arg(title));
    }

    QRectF content(QQuickItem* card)
    {
        return rectIn(named(card, QStringLiteral("appIcon")), card)
            .united(rectIn(named(card, QStringLiteral("appTitle")), card));
    }

    QRectF removeButton(QQuickItem* card)
    {
        return rectIn(named(card, QStringLiteral("appRemoveButton")), card);
    }
};

TEST_F(AppCardTest, TheIconAndAOneLineTitleAreCentredInTheTile)
{
    QQuickItem* card = tile(QStringLiteral("Stoper"));
    ASSERT_NE(card, nullptr);

    const QRectF block = content(card);
    EXPECT_NEAR(block.center().x(), card->width() / 2, 1.0);
    EXPECT_NEAR(block.center().y(), card->height() / 2, 1.0);
}

TEST_F(AppCardTest, ATwoLineTitleStaysCentredAsOneBlockWithTheIcon)
{
    QQuickItem* card = tile(QStringLiteral("Plan treningowy na cały tydzień"));
    ASSERT_NE(card, nullptr);

    EXPECT_EQ(named(card, QStringLiteral("appTitle"))->property("lineCount").toInt(), 2);
    const QRectF block = content(card);
    EXPECT_NEAR(block.center().x(), card->width() / 2, 1.0);
    EXPECT_NEAR(block.center().y(), card->height() / 2, 1.0);
}

TEST_F(AppCardTest, TheRemoveButtonSitsInTheTopRightCornerWithASmallInset)
{
    QQuickItem* card = tile(QStringLiteral("Stoper"));
    ASSERT_NE(card, nullptr);

    const QRectF button = removeButton(card);
    EXPECT_GT(card->width() - button.right(), 0.0);
    EXPECT_LE(card->width() - button.right(), 8.0);
    EXPECT_GT(button.top(), 0.0);
    EXPECT_LE(button.top(), 8.0);
}

TEST_F(AppCardTest, TheRemoveButtonTouchesNeitherTheIconNorTheTitleInTheNarrowestTile)
{
    for (const QString& title :
         { QStringLiteral("Stoper"), QStringLiteral("Plan treningowy na cały tydzień") })
    {
        QQuickItem* card = tile(title);
        ASSERT_NE(card, nullptr);

        const QRectF button = removeButton(card);
        EXPECT_FALSE(button.intersects(rectIn(named(card, QStringLiteral("appIcon")), card)))
            << title.toStdString();
        EXPECT_FALSE(button.intersects(rectIn(named(card, QStringLiteral("appTitle")), card)))
            << title.toStdString();
    }
}
