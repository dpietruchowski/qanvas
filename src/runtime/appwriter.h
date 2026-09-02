#pragma once

#include <QString>

class AppWriter
{
public:
    static QString appsRoot();
    static QString appDir(const QString& id);
    static bool isValidId(const QString& id);
    static QString write(const QString& id, const QString& qml, QString* error);
};
