#pragma once

#include <QString>

class SystemPrompt
{
public:
    static QString text();
    static QString mainText();
    static QString compactionRequest();
    static QString compactedContext(const QString& reply);
};
