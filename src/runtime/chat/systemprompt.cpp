#include "runtime/chat/systemprompt.h"

#include <QFile>
#include <QRegularExpression>

#include "runtime/host/appscreenshot.h"

namespace
{
const QString kSummaryPrefix = QStringLiteral("Summary of your earlier work on this app:\n");

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

QString SystemPrompt::compactionRequest()
{
    return resourceText(QStringLiteral(":/qanvas/compactprompt.txt"));
}

QString SystemPrompt::compactedContext(const QString& reply)
{
    static const QRegularExpression appLine(QStringLiteral("^App:"),
                                            QRegularExpression::MultilineOption);
    const QString summary = reply.trimmed();
    return appLine.match(summary).hasMatch() ? kSummaryPrefix + summary : QString();
}
