#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class QQmlEngine;
class QQmlError;

class AppWarnings : public QObject
{
    Q_OBJECT

public:
    explicit AppWarnings(QQmlEngine& engine, QObject* parent = nullptr);

    QStringList lines(const QString& appId) const { return m_lines.value(appId); }
    void clear(const QString& appId);
    void setCurrentApp(const QString& appId);

private:
    void record(const QList<QQmlError>& errors);

    QHash<QString, QStringList> m_lines;
    QString m_currentApp;
};
