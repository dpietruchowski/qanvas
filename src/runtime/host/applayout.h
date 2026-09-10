#pragma once

#include <QString>

class QQuickItem;

class AppLayout
{
public:
    static QString report(QQuickItem* root, const QString& appDir);
    static QString reportFor(const QString& appId);
};
