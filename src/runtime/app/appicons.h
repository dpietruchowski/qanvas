#pragma once

#include <memory>

#include <QObject>
#include <QString>
#include <QStringList>

class QQmlEngine;

class AppIcons
{
public:
    explicit AppIcons(QQmlEngine* engine);

    QString resolve(const QString& name) const;
    QStringList names() const;

private:
    std::unique_ptr<QObject> m_theme;
};
