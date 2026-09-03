#include "runtime/chat/systemprompt.h"

#include <QFile>

QString SystemPrompt::text()
{
    QFile file(QStringLiteral(":/qanvas/systemprompt.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll()).trimmed();
}
