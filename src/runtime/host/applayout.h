#pragma once

#include <QString>

class QQuickItem;

class AppLayout
{
public:
    struct Report
    {
        QString files;
        QString problems;

        bool fits() const { return problems.isEmpty(); }
        QString text() const;
    };

    static Report measure(QQuickItem* root, const QString& appDir);
    static Report measureApp(const QString& appId);
};
