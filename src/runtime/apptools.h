#pragma once

#include <functional>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "agent/agentcontroller.h"
#include "utils/result.h"

class App;
class AppFiles;
class AppRegistry;
class QQmlEngine;

class AppTools : public QObject
{
    Q_OBJECT

public:
    AppTools(AgentController& controller, AppRegistry& registry, QObject* parent = nullptr);

    void setEngine(QQmlEngine* engine);

    QVariantList activities() const { return m_activities; }
    void clearActivities();

signals:
    void activitiesChanged();
    void appShown(const QString& id);

private:
    struct Activity
    {
        const char* running;
        const char* done;
        const char* failed;
        const char* detail;
    };

    using FileOperation = std::function<Result<QString>(AppFiles&, const QVariantMap&)>;

    void registerTools();
    void addCreateAppTool();
    void addFileTool(const char* docstring, const Activity& activity, bool reloads,
                     const FileOperation& operation);
    Result<QString> show(const App& app, const QString& result);
    int beginActivity(const Activity& activity, const QVariantMap& arguments);
    void endActivity(int index, bool ok);
    QString unknownApp(const QString& id) const;

    AgentController& m_controller;
    AppRegistry& m_registry;
    QQmlEngine* m_engine = nullptr;
    QVariantList m_activities;
};
