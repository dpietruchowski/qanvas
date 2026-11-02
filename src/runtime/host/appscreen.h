#pragma once

#include <QPointer>
#include <QString>
#include <QVariant>

class QQuickItem;

class AppScreen
{
public:
    AppScreen(const QString& appId, const QString& screen);
    ~AppScreen();

    AppScreen(const AppScreen&) = delete;
    AppScreen& operator=(const AppScreen&) = delete;

    QString error() const { return m_error; }

private:
    QPointer<QQuickItem> m_root;
    QVariant m_previous;
    QString m_error;
    bool m_changed = false;
};
