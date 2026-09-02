#include "apptools.h"

#include "app.h"
#include "appcompiler.h"
#include "appfiles.h"
#include "appregistry.h"

#include <QStringList>

namespace
{
const char* kCreateAppDocstring
    = "@brief create_app\n"
      "@details Creates an app from its Main.qml and runs it at once, replacing an app of the same "
      "id\n"
      "@param id (string) lowercase slug naming the app, 1-32 characters of a-z, 0-9 and -\n"
      "@param title (string) short human name for the app list\n"
      "@param qml (string) the complete Main.qml source\n"
      "@return ok, or the reason the app was rejected or does not load";

const char* kListFilesDocstring = "@brief list_files\n"
                                  "@details Lists the source files of an app with their lengths\n"
                                  "@param app (string) app id\n"
                                  "@return one \"Name.qml 12L\" line per file";

const char* kReadFileDocstring
    = "@brief read_file\n"
      "@details Reads one source file of an app, numbered by line\n"
      "@param app (string) app id\n"
      "@param path (string) file name, such as Main.qml\n"
      "@return the numbered file, or the names of the files the app does have";

const char* kSearchFilesDocstring
    = "@brief search_files\n"
      "@details Finds literal text across the source files of an app\n"
      "@param app (string) app id\n"
      "@param text (string) exact text to find, not a regular expression\n"
      "@return every matching line with the two lines around it";

const char* kWriteFileDocstring
    = "@brief write_file\n"
      "@details Writes a whole source file of an app, creating or overwriting it, and reloads the "
      "app\n"
      "@param app (string) app id\n"
      "@param path (string) Name.qml for a component, or name.js\n"
      "@param content (string) the complete file\n"
      "@return the file and its length, or the reason it was rejected or the app does not load";

const char* kEditFileDocstring
    = "@brief edit_file\n"
      "@details Replaces one exact fragment of a source file and reloads the app\n"
      "@param app (string) app id\n"
      "@param path (string) file name\n"
      "@param before (string) the exact text to replace, long enough to occur once in the file\n"
      "@param after (string) the text to put in its place\n"
      "@return the changed lines, the lines where the fragment repeats, or why the app does not "
      "load";

const char* kDeleteFileDocstring
    = "@brief delete_file\n"
      "@details Deletes a source file of an app and reloads it; Main.qml cannot be deleted\n"
      "@param app (string) app id\n"
      "@param path (string) file name\n"
      "@return confirmation, or the reason it was refused or the app does not load";

QString argument(const QVariantMap& arguments, const char* name)
{
    return arguments.value(QString::fromLatin1(name)).toString();
}

QString invalidId(const QString& id)
{
    return QStringLiteral("Invalid app id \"%1\": use 1-32 characters of a-z, 0-9 and -").arg(id);
}

QString doesNotLoad(const QString& error)
{
    return QStringLiteral("Saved, but the app does not load:\n%1").arg(error);
}
}

AppTools::AppTools(AgentController& controller, AppRegistry& registry, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_registry(registry)
{
    registerTools();
}

void AppTools::setEngine(QQmlEngine* engine) { m_engine = engine; }

void AppTools::clearActivities()
{
    if (m_activities.isEmpty())
        return;

    m_activities.clear();
    emit activitiesChanged();
}

void AppTools::registerTools()
{
    addCreateAppTool();

    addFileTool(kListFilesDocstring,
                { "listing files in", "listed files in", "could not list files in", "app" }, false,
                [](AppFiles& files, const QVariantMap&) { return files.list(); });

    addFileTool(kReadFileDocstring, { "reading", "read", "could not read", "path" }, false,
                [](AppFiles& files, const QVariantMap& arguments)
                { return files.read(argument(arguments, "path")); });

    addFileTool(kSearchFilesDocstring,
                { "searching for", "searched for", "could not search for", "text" }, false,
                [](AppFiles& files, const QVariantMap& arguments)
                { return files.grep(argument(arguments, "text")); });

    addFileTool(kWriteFileDocstring, { "writing", "wrote", "could not write", "path" }, true,
                [](AppFiles& files, const QVariantMap& arguments) {
                    return files.write(argument(arguments, "path"), argument(arguments, "content"));
                });

    addFileTool(kEditFileDocstring, { "editing", "edited", "could not edit", "path" }, true,
                [](AppFiles& files, const QVariantMap& arguments)
                {
                    return files.replace(argument(arguments, "path"), argument(arguments, "before"),
                                         argument(arguments, "after"));
                });

    addFileTool(kDeleteFileDocstring, { "deleting", "deleted", "could not delete", "path" }, true,
                [](AppFiles& files, const QVariantMap& arguments)
                { return files.remove(argument(arguments, "path")); });
}

void AppTools::addCreateAppTool()
{
    m_controller.addTool(
        QString::fromLatin1(kCreateAppDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "building", "built", "could not build", "title" };
            const int index = beginActivity(activity, arguments);
            const QString id = argument(arguments, "id");

            const App app(id);
            if (!app.isValid())
            {
                endActivity(index, false);
                done(invalidId(id));
                return;
            }

            const Result<QString> written
                = AppFiles(app.dir()).write(QStringLiteral("Main.qml"), argument(arguments, "qml"));
            if (written.isFailure())
            {
                endActivity(index, false);
                done(written.error());
                return;
            }

            m_registry.record(id, argument(arguments, "title"));

            const Result<QString> shown = show(app, QStringLiteral("ok"));
            endActivity(index, shown.isSuccess());
            done(shown.isSuccess() ? shown.value() : shown.error());
        });
}

void AppTools::addFileTool(const char* docstring, const Activity& activity, bool reloads,
                           const FileOperation& operation)
{
    m_controller.addTool(QString::fromLatin1(docstring),
                         [this, activity, reloads, operation](QVariantMap arguments, ToolDone done)
                         {
                             const int index = beginActivity(activity, arguments);
                             const QString id = argument(arguments, "app");
                             const App app(id);
                             if (!app.isValid())
                             {
                                 endActivity(index, false);
                                 done(invalidId(id));
                                 return;
                             }

                             if (!app.exists())
                             {
                                 endActivity(index, false);
                                 done(unknownApp(id));
                                 return;
                             }

                             AppFiles files(app.dir());
                             const Result<QString> result = operation(files, arguments);
                             if (result.isFailure())
                             {
                                 endActivity(index, false);
                                 done(result.error());
                                 return;
                             }

                             if (!reloads)
                             {
                                 endActivity(index, true);
                                 done(result.value());
                                 return;
                             }

                             const Result<QString> shown = show(app, result.value());
                             endActivity(index, shown.isSuccess());
                             done(shown.isSuccess() ? shown.value() : shown.error());
                         });
}

Result<QString> AppTools::show(const App& app, const QString& result)
{
    const Result<void> compiled
        = m_engine ? AppCompiler(*m_engine).compile(app) : Result<void>::success();
    emit appShown(app.id());

    if (compiled.isFailure())
        return Result<QString>::failure(doesNotLoad(compiled.error()));
    return Result<QString>::success(result);
}

int AppTools::beginActivity(const Activity& activity, const QVariantMap& arguments)
{
    m_activities.append(QVariantMap {
        { QStringLiteral("text"), QString::fromLatin1(activity.running) },
        { QStringLiteral("done"), QString::fromLatin1(activity.done) },
        { QStringLiteral("failedText"), QString::fromLatin1(activity.failed) },
        { QStringLiteral("detail"), argument(arguments, activity.detail).simplified() },
        { QStringLiteral("running"), true },
        { QStringLiteral("failed"), false } });
    emit activitiesChanged();
    return int(m_activities.size()) - 1;
}

void AppTools::endActivity(int index, bool ok)
{
    if (index < 0 || index >= m_activities.size())
        return;

    QVariantMap entry = m_activities.at(index).toMap();
    entry[QStringLiteral("text")]
        = ok ? entry.value(QStringLiteral("done")) : entry.value(QStringLiteral("failedText"));
    entry[QStringLiteral("running")] = false;
    entry[QStringLiteral("failed")] = !ok;
    m_activities[index] = entry;
    emit activitiesChanged();
}

QString AppTools::unknownApp(const QString& id) const
{
    QStringList ids;
    const QVariantList apps = m_registry.apps();
    for (const QVariant& app : apps)
        ids.append(app.toMap().value(QStringLiteral("id")).toString());

    if (ids.isEmpty())
        return QStringLiteral("No app \"%1\", and there are no apps yet").arg(id);
    return QStringLiteral("No app \"%1\" — apps: %2").arg(id, ids.join(QLatin1Char(' ')));
}
