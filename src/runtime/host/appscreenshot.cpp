#include "runtime/host/appscreenshot.h"

#include <QBuffer>
#include <QEventLoop>
#include <QImage>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QSharedPointer>
#include <QTimer>
#include <QtGlobal>

#include "runtime/host/apploader.h"

namespace
{
constexpr const char* kFlag = "QANVAS_DEV_SCREENSHOT";
constexpr int kJpegQuality = 80;
constexpr int kGrabTimeoutMs = 3000;

QByteArray encoded(const QImage& image, const char* format, int quality)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, format, quality))
        return QByteArray();
    return bytes;
}

QImage fitted(const QImage& image)
{
    if (qMax(image.width(), image.height()) <= AppScreenshot::kMaxSide)
        return image;
    return image.scaled(AppScreenshot::kMaxSide, AppScreenshot::kMaxSide, Qt::KeepAspectRatio,
                        Qt::SmoothTransformation);
}
}

bool AppScreenshot::enabled() { return qEnvironmentVariable(kFlag) == QStringLiteral("1"); }

ToolImage AppScreenshot::encode(const QImage& area, const QString& caption)
{
    const QImage picture = fitted(area);
    const QByteArray png = encoded(picture, "PNG", -1);
    const QByteArray jpeg
        = encoded(picture.convertToFormat(QImage::Format_RGB32), "JPEG", kJpegQuality);

    const bool useJpeg = !jpeg.isEmpty() && (png.isEmpty() || jpeg.size() < png.size());
    ToolImage image;
    image.mimeType = QString::fromLatin1(useJpeg ? "image/jpeg" : "image/png");
    image.data = useJpeg ? jpeg : png;
    image.text
        = QStringLiteral("%1 (%2x%3 px)").arg(caption).arg(picture.width()).arg(picture.height());
    return image;
}

Result<ToolImage> AppScreenshot::capture(const QString& appId)
{
    AppLoader* loader = AppLoader::running(appId);
    if (!loader || !loader->window())
        return Result<ToolImage>::failure(
            QStringLiteral("\"%1\" is not on screen, so it cannot be photographed").arg(appId));

    const QSharedPointer<QQuickItemGrabResult> grab = loader->grabToImage();
    if (!grab)
        return Result<ToolImage>::failure(
            QStringLiteral("\"%1\" has no visible area to photograph").arg(appId));

    QEventLoop loop;
    QObject::connect(grab.data(), &QQuickItemGrabResult::ready, &loop, &QEventLoop::quit);
    QTimer::singleShot(kGrabTimeoutMs, &loop, &QEventLoop::quit);
    loop.exec();

    const QImage picture = grab->image();
    if (picture.isNull())
        return Result<ToolImage>::failure(
            QStringLiteral("\"%1\" could not be photographed, try again").arg(appId));

    return Result<ToolImage>::success(
        encode(picture, QStringLiteral("Screenshot of \"%1\"").arg(appId)));
}
