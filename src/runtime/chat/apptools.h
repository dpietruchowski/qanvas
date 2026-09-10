#pragma once

#include <functional>
#include <memory>
#include <optional>

#include <QObject>
#include <QSet>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "agent/agentcontroller.h"
#include "async/result.h"
#include "runtime/app/app.h"
#include "runtime/app/appfiles.h"

class AppRegistry;
class AppWarnings;
class QQmlEngine;

class AppTools : public QObject
{
    Q_OBJECT

public:
    AppTools(AgentController& controller, AppRegistry& registry, QObject* parent = nullptr);
    ~AppTools() override;

    void setEngine(QQmlEngine* engine);

    QVariantList activities() const { return m_activities; }
    void clearActivities();
    void setCurrentApp(const QString& id);
    QString unfinished(const App& app, bool thisTurnOnly = false) const;

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
    void addDeleteAppTool();
    void addDescribeAppTool();
    void addReadLogTool();
    void addCheckLayoutTool();
    void addGrantAccessTool();
    void addFileTool(const char* docstring, const Activity& activity, bool reloads,
                     const FileOperation& operation);
    std::optional<QString> refusal(const App& app) const;
    App appFor(const QVariantMap& arguments, QString* note) const;
    void finish(int index, const Result<QString>& result, const ToolDone& done,
                const QString& note = QString());
    void show(const App& app, const QString& result, int index, const ToolDone& done,
              const AppSources& restoreTo = AppSources(), const QString& priorError = QString());
    QString currentError(const App& app) const;
    int beginActivity(const Activity& activity, const QVariantMap& arguments);
    void endActivity(int index, bool ok);
    void noteOutcome(const QString& key, const QString& answer);
    QString unknownApp(const QString& id) const;

    AgentController& m_controller;
    AppRegistry& m_registry;
    QQmlEngine* m_engine = nullptr;
    std::unique_ptr<AppWarnings> m_warnings;
    QVariantList m_activities;
    QSet<QString> m_rejected;
    QSet<QString> m_written;
    QString m_currentApp;
};
