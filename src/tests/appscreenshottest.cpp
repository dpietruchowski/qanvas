#include <gtest/gtest.h>

#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickView>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QtQml>

#include "runtime/host/apploader.h"
#include "runtime/host/appscreenshot.h"

namespace
{
QImage solid(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(QColor(QStringLiteral("#2040c0")));
    return image;
}

QImage noisy(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    quint32 state = 12345;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            state = state * 1664525u + 1013904223u;
            image.setPixel(x, y, state | 0xff000000u);
        }
    }
    return image;
}

QImage decoded(const ToolImage& image) { return QImage::fromData(image.data); }

class AppScreenshotFlagTest : public ::testing::Test
{
protected:
    void TearDown() override { qunsetenv("QANVAS_DEV_SCREENSHOT"); }
};

class AppScreenshotCaptureTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        qputenv("QANVAS_APPS_ROOT", root().path().toLocal8Bit());
        qmlRegisterType<AppLoader>("ShotHost", 1, 0, "AppLoader");
    }

    static void TearDownTestSuite() { qunsetenv("QANVAS_APPS_ROOT"); }

    static QTemporaryDir& root()
    {
        static QTemporaryDir directory;
        return directory;
    }

    void writeApp(const QString& id, const QString& body)
    {
        ASSERT_TRUE(QDir().mkpath(root().path() + QLatin1Char('/') + id));
        QFile file(root().path() + QLatin1Char('/') + id + QStringLiteral("/Main.qml"));
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(body.toUtf8());
    }

    void show(const QString& host)
    {
        const QString path = root().path() + QStringLiteral("/Host.qml");
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(host.toUtf8());
        file.close();

        view.resize(300, 400);
        view.setSource(QUrl::fromLocalFile(path));
        ASSERT_EQ(view.status(), QQuickView::Ready);
        view.show();
        ASSERT_TRUE(QTest::qWaitForWindowExposed(&view));
        QTest::qWait(50);
    }

    QQuickView view;
};
}

TEST_F(AppScreenshotFlagTest, IsOffWhenTheVariableIsNotSet)
{
    qunsetenv("QANVAS_DEV_SCREENSHOT");

    EXPECT_FALSE(AppScreenshot::enabled());
}

TEST_F(AppScreenshotFlagTest, IsOnOnlyForTheValueOne)
{
    qputenv("QANVAS_DEV_SCREENSHOT", "1");
    EXPECT_TRUE(AppScreenshot::enabled());

    qputenv("QANVAS_DEV_SCREENSHOT", "0");
    EXPECT_FALSE(AppScreenshot::enabled());

    qputenv("QANVAS_DEV_SCREENSHOT", "");
    EXPECT_FALSE(AppScreenshot::enabled());
}

TEST(AppScreenshotEncodeTest, ScalesALargeAreaSoItsLongSideIsAtMostTheLimit)
{
    const ToolImage image = AppScreenshot::encode(solid(2000, 1200), QStringLiteral("caption"));

    const QImage picture = decoded(image);
    ASSERT_FALSE(picture.isNull());
    EXPECT_EQ(picture.width(), AppScreenshot::kMaxSide);
    EXPECT_EQ(picture.height(), 480);
}

TEST(AppScreenshotEncodeTest, ScalesATallAreaByItsHeight)
{
    const ToolImage image = AppScreenshot::encode(solid(900, 3000), QStringLiteral("caption"));

    const QImage picture = decoded(image);
    ASSERT_FALSE(picture.isNull());
    EXPECT_EQ(picture.height(), AppScreenshot::kMaxSide);
    EXPECT_EQ(picture.width(), 240);
}

TEST(AppScreenshotEncodeTest, KeepsAnAreaThatAlreadyFitsAtItsSize)
{
    const ToolImage image = AppScreenshot::encode(solid(300, 500), QStringLiteral("caption"));

    const QImage picture = decoded(image);
    EXPECT_EQ(picture.size(), QSize(300, 500));
}

TEST(AppScreenshotEncodeTest, NamesThePictureAndItsSizeInTheCaption)
{
    const ToolImage image = AppScreenshot::encode(solid(300, 500), QStringLiteral("Shot of x"));

    EXPECT_EQ(image.text, QStringLiteral("Shot of x (300x500 px)"));
}

TEST(AppScreenshotEncodeTest, SendsAFlatPictureAsPng)
{
    const ToolImage image = AppScreenshot::encode(solid(400, 400), QStringLiteral("caption"));

    EXPECT_EQ(image.mimeType, QStringLiteral("image/png"));
}

TEST(AppScreenshotEncodeTest, SendsAPictureOfNoiseAsJpeg)
{
    const ToolImage image = AppScreenshot::encode(noisy(400, 400), QStringLiteral("caption"));

    EXPECT_EQ(image.mimeType, QStringLiteral("image/jpeg"));
    EXPECT_FALSE(decoded(image).isNull());
}

TEST_F(AppScreenshotCaptureTest, AnAppThatIsNotOnScreenCannotBePhotographed)
{
    const Result<ToolImage> shot = AppScreenshot::capture(QStringLiteral("nowhere"));

    ASSERT_TRUE(shot.isFailure());
    EXPECT_TRUE(shot.error().contains(QStringLiteral("not on screen"))) << qPrintable(shot.error());
}

TEST_F(AppScreenshotCaptureTest, PhotographsOnlyTheAreaOfTheRunningApp)
{
    writeApp(QStringLiteral("shot"),
             QStringLiteral("import QtQuick\nItem { Rectangle { anchors.fill: parent; color: "
                            "\"#2040c0\" } }\n"));
    show(QStringLiteral(
        "import QtQuick\nimport ShotHost\nItem {\n    width: 300; height: 400\n"
        "    Rectangle { anchors.fill: parent; color: \"#ff0000\" }\n    "
        "AppLoader { x: 20; y: 30; width: 100; height: 150; appId: \"shot\" }\n}\n"));

    const Result<ToolImage> shot = AppScreenshot::capture(QStringLiteral("shot"));

    ASSERT_TRUE(shot.isSuccess()) << qPrintable(shot.error());
    const QImage picture = decoded(shot.value());
    ASSERT_FALSE(picture.isNull());
    EXPECT_EQ(picture.size(), QSize(100, 150));
    const QColor middle = picture.pixelColor(50, 75);
    EXPECT_EQ(middle.blue(), 0xc0);
    EXPECT_EQ(middle.red(), 0x20);
    EXPECT_TRUE(shot.value().text.contains(QStringLiteral("\"shot\"")));
}
