#include "runtime/chat/systemprompt.h"

#include <QFile>

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

QString SystemPrompt::text() { return resourceText(QStringLiteral(":/qanvas/systemprompt.txt")); }

QString SystemPrompt::mainText() { return resourceText(QStringLiteral(":/qanvas/chatprompt.txt")); }
