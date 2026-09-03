#include "runtime/chat/apptools.h"

#include "runtime/app/app.h"
#include "runtime/app/appcompiler.h"
#include "runtime/app/appfiles.h"
#include "runtime/app/appregistry.h"
#include "runtime/app/appschema.h"
#include "runtime/app/appwarnings.h"
#include "runtime/host/notesbridge.h"
#include "runtime/host/tasksbridge.h"

#include <QStringList>
#include <QTimer>

namespace
{
const int kSettleMs = 300;

const char* kCreateAppDocstring
    = "@brief create_app\n"
      "@details Creates an app from its Main.qml and runs it at once, replacing an app of the same "
      "id\n"
      "@param id (string) lowercase slug naming the app, 1-32 characters of a-z, 0-9 and -\n"
      "@param title (string) short human name for the app list\n"
      "@param qml (string) the complete Main.qml source\n"
      "@return ok, or the reason the app was rejected, does not load, or raised errors while "
      "loading";

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
      "@return the file and its length, or the reason it was rejected, the app does not load, or "
      "the errors it raised while loading";

const char* kEditFileDocstring
    = "@brief edit_file\n"
      "@details Replaces one exact fragment of a source file and reloads the app\n"
      "@param app (string) app id\n"
      "@param path (string) file name\n"
      "@param before (string) the exact text to replace, long enough to occur once in the file\n"
      "@param after (string) the text to put in its place\n"
      "@return the changed lines, the lines where the fragment repeats, why the app does not "
      "load, or the errors it raised while loading";

const char* kDeleteFileDocstring
    = "@brief delete_file\n"
      "@details Deletes a source file of an app and reloads it; Main.qml cannot be deleted\n"
      "@param app (string) app id\n"
      "@param path (string) file name\n"
      "@return confirmation, or the reason it was refused, the app does not load, or the errors "
      "it raised while loading";

const char* kDeleteAppDocstring
    = "@brief delete_app\n"
      "@details Deletes an app for good: its files, its database and its place in the app list; "
      "only when the user asked to remove that app\n"
      "@param app (string) app id\n"
      "@return confirmation, or the reason it was refused";

const char* kDescribeAppDocstring
    = "@brief describe_app\n"
      "@details Shows what an app is made of: every source file with the functions it defines, "
      "and every table in the app's database with its CREATE statement and row count\n"
      "@param app (string) app id\n"
      "@return the files with their functions, then the tables";

const char* kReadLogDocstring
    = "@brief read_log\n"
      "@details Lists the runtime errors an app has raised since it last loaded: failed bindings, "
      "exceptions in handlers and timers, including ones the user triggered by using the app\n"
      "@param app (string) app id\n"
      "@return one \"File.qml:line:column: message\" line per error, oldest first";

const char* kGrantAccessDocstring
    = "@brief grant_access\n"
      "@details Lets an app use one of the user's shared stores and reloads it; only when the "
      "user asked for an app that works on that data\n"
      "@param app (string) app id\n"
      "@param store (string) the store to share: notes or tasks\n"
      "@return confirmation, or the reason it was refused";

QStringList sharedStores() { return { NotesBridge::capability(), TasksBridge::capability() }; }

QString storeObject(const QString& store) { return store.left(1).toUpper() + store.mid(1); }

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

QString raisedWhileLoading(const QStringList& lines)
{
    return QStringLiteral("Saved and loaded, but the app raised:\n%1")
        .arg(lines.join(QLatin1Char('\n')));
}

QString logText(const QStringList& lines)
{
    if (lines.isEmpty())
        return QStringLiteral("No errors since the app last loaded");
    return lines.join(QLatin1Char('\n'));
}
}

AppTools::AppTools(AgentController& controller, AppRegistry& registry, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_registry(registry)
{
    registerTools();
}

AppTools::~AppTools() = default;

void AppTools::setEngine(QQmlEngine* engine)
{
    m_engine = engine;
    m_warnings = engine ? std::make_unique<AppWarnings>(*engine) : nullptr;
}

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

    addDescribeAppTool();
    addDeleteAppTool();
    addReadLogTool();
    addGrantAccessTool();
}

void AppTools::addDescribeAppTool()
{
    m_controller.addTool(QString::fromLatin1(kDescribeAppDocstring),
                         [this](QVariantMap arguments, ToolDone done)
                         {
                             const Activity activity { "inspecting", "inspected",
                                                       "could not inspect", "app" };
                             const int index = beginActivity(activity, arguments);
                             const App app(argument(arguments, "app"));
                             if (const std::optional<QString> refused = refusal(app))
                             {
                                 finish(index, Result<QString>::failure(*refused), done);
                                 return;
                             }

                             const Result<QString> files = AppFiles(app.dir()).outline();
                             if (files.isFailure())
                             {
                                 finish(index, files, done);
                                 return;
                             }

                             finish(index,
                                    Result<QString>::success(files.value() + QStringLiteral("\n\n")
                                                             + AppSchema(app).describe()),
                                    done);
                         });
}

void AppTools::addGrantAccessTool()
{
    m_controller.addTool(
        QString::fromLatin1(kGrantAccessDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "sharing", "shared", "could not share", "store" };
            const int index = beginActivity(activity, arguments);
            const App app(argument(arguments, "app"));
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            const QString store = argument(arguments, "store");
            if (!sharedStores().contains(store))
            {
                finish(index,
                       Result<QString>::failure(
                           QStringLiteral("No store \"%1\": choose %2")
                               .arg(store, sharedStores().join(QStringLiteral(" or ")))),
                       done);
                return;
            }

            if (!m_registry.grant(app.id(), store))
            {
                finish(index,
                       Result<QString>::failure(
                           QStringLiteral("Could not share %1 with \"%2\"").arg(store, app.id())),
                       done);
                return;
            }

            show(app, QStringLiteral("\"%1\" may use %2 now").arg(app.id(), storeObject(store)),
                 index, done);
        });
}

void AppTools::addDeleteAppTool()
{
    m_controller.addTool(
        QString::fromLatin1(kDeleteAppDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "deleting", "deleted", "could not delete", "app" };
            const int index = beginActivity(activity, arguments);
            const App app(argument(arguments, "app"));
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            if (!m_registry.remove(app.id()))
            {
                finish(index,
                       Result<QString>::failure(
                           QStringLiteral("Could not delete \"%1\"").arg(app.id())),
                       done);
                return;
            }

            finish(index, Result<QString>::success(QStringLiteral("Deleted \"%1\"").arg(app.id())),
                   done);
        });
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
                finish(index, Result<QString>::failure(invalidId(id)), done);
                return;
            }

            const Result<QString> written
                = AppFiles(app.dir()).write(QStringLiteral("Main.qml"), argument(arguments, "qml"));
            if (written.isFailure())
            {
                finish(index, written, done);
                return;
            }

            m_registry.record(id, argument(arguments, "title"));
            show(app, QStringLiteral("ok"), index, done);
        });
}

void AppTools::addReadLogTool()
{
    m_controller.addTool(
        QString::fromLatin1(kReadLogDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "checking", "checked", "could not check", "app" };
            const int index = beginActivity(activity, arguments);
            const App app(argument(arguments, "app"));
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            const QStringList lines = m_warnings ? m_warnings->lines(app.id()) : QStringList();
            finish(index, Result<QString>::success(logText(lines)), done);
        });
}

void AppTools::addFileTool(const char* docstring, const Activity& activity, bool reloads,
                           const FileOperation& operation)
{
    m_controller.addTool(QString::fromLatin1(docstring),
                         [this, activity, reloads, operation](QVariantMap arguments, ToolDone done)
                         {
                             const int index = beginActivity(activity, arguments);
                             const App app(argument(arguments, "app"));
                             if (const std::optional<QString> refused = refusal(app))
                             {
                                 finish(index, Result<QString>::failure(*refused), done);
                                 return;
                             }

                             AppFiles files(app.dir());
                             const Result<QString> result = operation(files, arguments);
                             if (result.isFailure() || !reloads)
                             {
                                 finish(index, result, done);
                                 return;
                             }

                             show(app, result.value(), index, done);
                         });
}

std::optional<QString> AppTools::refusal(const App& app) const
{
    if (!app.isValid())
        return invalidId(app.id());
    if (!app.exists())
        return unknownApp(app.id());
    return std::nullopt;
}

void AppTools::finish(int index, const Result<QString>& result, const ToolDone& done)
{
    endActivity(index, result.isSuccess());
    done(result.isSuccess() ? result.value() : result.error());
}

void AppTools::show(const App& app, const QString& result, int index, const ToolDone& done)
{
    const Result<void> compiled
        = m_engine ? AppCompiler(*m_engine).compile(app) : Result<void>::success();
    if (m_warnings)
        m_warnings->clear(app.id());
    emit appShown(app.id());

    if (compiled.isFailure())
    {
        finish(index, Result<QString>::failure(doesNotLoad(compiled.error())), done);
        return;
    }

    if (!m_warnings)
    {
        finish(index, Result<QString>::success(result), done);
        return;
    }

    QTimer::singleShot(kSettleMs, this,
                       [this, id = app.id(), result, index, done]
                       {
                           const QStringList raised = m_warnings->lines(id);
                           if (raised.isEmpty())
                               finish(index, Result<QString>::success(result), done);
                           else
                               finish(index, Result<QString>::failure(raisedWhileLoading(raised)),
                                      done);
                       });
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
