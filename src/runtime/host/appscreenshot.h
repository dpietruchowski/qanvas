#pragma once

#include <QString>

#include "agent/structures.h"
#include "async/result.h"

class QImage;

class AppScreenshot
{
public:
    static constexpr int kMaxSide = 800;

    static bool enabled();
    static ToolImage encode(const QImage& area, const QString& caption);
    static Result<ToolImage> capture(const QString& appId);
};
