#include "runtime/chat/systemprompt.h"

#include <QFile>

#include "runtime/host/appscreenshot.h"

namespace
{
QString resourceText(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll()).trimmed();
}
}

QString SystemPrompt::text()
{
    const QString prompt = resourceText(QStringLiteral(":/qanvas/systemprompt.txt"));
    if (!AppScreenshot::enabled())
        return prompt;
    return prompt + QStringLiteral("\n\n")
        + resourceText(QStringLiteral(":/qanvas/devscreenshotprompt.txt"));
}

QString SystemPrompt::mainText() { return resourceText(QStringLiteral(":/qanvas/chatprompt.txt")); }
