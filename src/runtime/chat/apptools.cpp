#include "runtime/chat/apptools.h"

#include "agent/agent.h"
#include "runtime/app/app.h"
#include "runtime/app/appcompiler.h"
#include "runtime/app/appfiles.h"
#include "runtime/app/appicons.h"
#include "runtime/app/appregistry.h"
#include "runtime/app/appschema.h"
#include "runtime/app/appwarnings.h"
#include "runtime/host/applayout.h"
#include "runtime/host/apploader.h"
#include "runtime/host/appscreen.h"
#include "runtime/host/appscreenshot.h"
#include "runtime/host/notesbridge.h"
#include "runtime/host/tasksbridge.h"

#include <algorithm>

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTimer>

namespace
{
const int kSettleMs = 500;

const char* kCreateAppDocstring
    = "@brief create_app\n"
      "@details Creates an app from its Main.qml and Data.qml and runs it at once, replacing an "
      "app of the same id\n"
      "@param id (string) lowercase slug naming the app, 1-32 characters of a-z, 0-9 and -\n"
      "@param title (string) short human name for the app list\n"
      "@param icon (string) the app's icon in the app list, one of the iconName names\n"
      "@param qml (string) the complete Main.qml source\n"
      "@param data (string) the complete Data.qml source, or an empty string when the app keeps "
      "no data\n"
      "@return ok, or the reason the app was rejected, does not load, or raised errors while "
      "loading";

const char* kListAppsDocstring
    = "@brief list_apps\n"
      "@details Lists the apps that exist, to find out whether one already does the job before "
      "building another\n"
      "@return one \"id  title\" line per app, most recently changed first, or that there are no "
      "apps yet";

const char* kListFilesDocstring = "@brief list_files\n"
                                  "@details Lists the source files of an app with their lengths\n"
                                  "@param app (string) app id\n"
                                  "@return one \"Name.qml 12L\" line per file";

const char* kReadFileDocstring
    = "@brief read_file\n"
      "@details Reads one source file of an app, numbered by line\n"
      "@param app (string) app id\n"
      "@param path (string) file name, such as Main.qml\n"
      "@param from (integer) first line to read, 0 or 1 for the start of the file\n"
      "@param lines (integer) how many lines to read, 0 for the whole file\n"
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
      "@return one \"File.qml:line:column: message\" line per error, oldest first, then the "
      "writes rejected this turn and never redone, the files no other file uses, and the calls "
      "to functions Data.qml does not define";

const char* kCheckLayoutDocstring
    = "@brief check_layout\n"
      "@details Measures the running app on screen and lists every element that sticks out of "
      "the screen, is cut off by the screen or by a card or scroll around it, has a text that "
      "does not fit, is drawn over another element, or has no size; use it after the user says "
      "something is off screen, not centred, overlapping or missing\n"
      "@param app (string) app id\n"
      "@param screen (string) the screen to measure, the name the app's screen property takes, "
      "or an empty string for the screen that is open\n"
      "@return the files measured, then the screen size with one line per layout problem, or "
      "that the layout fits";

const char* kScreenshotDocstring
    = "@brief screenshot\n"
      "@details Takes a picture of the running app on screen, the way the user sees it; use it "
      "after you built or changed a screen, once the app has loaded\n"
      "@param app (string) app id\n"
      "@param screen (string) the screen to photograph, the name the app's screen property takes, "
      "or an empty string for the screen that is open\n"
      "@return the picture of the app, or why it could not be taken";

const char* kGrantAccessDocstring
    = "@brief grant_access\n"
      "@details Lets an app use one of the user's shared stores and reloads it; only when the "
      "user asked for an app that works on that data\n"
      "@param app (string) app id\n"
      "@param store (string) the store to share: notes or tasks\n"
      "@return confirmation, or the reason it was refused";

const char* kSetAppIconDocstring
    = "@brief set_app_icon\n"
      "@details Changes the icon an app has in the app list, without touching its files\n"
      "@param app (string) app id\n"
      "@param icon (string) the new icon, one of the iconName names\n"
      "@return confirmation, or the reason it was refused";

const char* kNoteDocstring
    = "@brief note\n"
      "@details Writes one entry into the builder's notebook, which the developers read later to "
      "improve the runtime and this prompt; the user never sees it. Call it as the last step of "
      "every turn that built, changed or fixed an app, and at once when the same error comes back "
      "twice\n"
      "@param app (string) app id\n"
      "@param text (string) short and concrete, in English: what took retries and why, which "
      "components, component properties or Data and runtime functions were missing, which rule of "
      "the prompt misled you; when nothing was missing, one line saying so and what was hardest\n"
      "@return ok, or why the note could not be saved";

QStringList sharedStores() { return { NotesBridge::capability(), TasksBridge::capability() }; }

QString storeObject(const QString& store) { return store.left(1).toUpper() + store.mid(1); }

QString argument(const QVariantMap& arguments, const char* name)
{
    return arguments.value(QString::fromLatin1(name)).toString();
}

int number(const QVariantMap& arguments, const char* name)
{
    return arguments.value(QString::fromLatin1(name)).toInt();
}

QString invalidId(const QString& id)
{
    return QStringLiteral("Invalid app id \"%1\": use 1-32 characters of a-z, 0-9 and -").arg(id);
}

QString doesNotLoad(const QString& error)
{
    return QStringLiteral("Saved, but the app does not load:\n%1").arg(error);
}

QString wasRolledBack(const QString& error)
{
    return QStringLiteral("Rejected and undone — the app still runs the version it ran before. "
                          "It would not have loaded:\n%1")
        .arg(error);
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

QStringList brokenFiles(const QString& errors)
{
    QStringList files;
    const QStringList lines = errors.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines)
        files.append(line.section(QLatin1Char(':'), 0, 0));
    return files;
}

bool onlyOldErrors(const QString& errors, const QString& priorErrors)
{
    if (priorErrors.isEmpty())
        return false;
    const QStringList before = brokenFiles(priorErrors);
    const QStringList after = brokenFiles(errors);
    for (const QString& file : after)
    {
        if (!before.contains(file))
            return false;
    }
    return true;
}

QString explained(const QString& errors, const QString& dir)
{
    static const QRegularExpression missingType(QStringLiteral("\\b([A-Z]\\w*) is not a type\\b"));
    QStringList lines = errors.split(QLatin1Char('\n'));
    for (QString& line : lines)
    {
        const QRegularExpressionMatch match = missingType.match(line);
        if (!match.hasMatch())
            continue;
        const QString name = match.captured(1);
        if (name.startsWith(QStringLiteral("Qanvas")))
            line += QStringLiteral(" (no such component: see COMPONENTS)");
        else if (!QFile::exists(dir + QLatin1Char('/') + name + QStringLiteral(".qml")))
            line += QStringLiteral(" (there is no %1.qml in the app: write_file it first)")
                        .arg(name);
        else
            line += QStringLiteral(" (%1.qml is there but does not load on its own)").arg(name);
    }
    return lines.join(QLatin1Char('\n'));
}

QString missingFunctions(const QStringList& calls)
{
    return QStringLiteral("Saved and loaded, but it will throw when that screen opens:\n%1\nAdd "
                          "the function to Data.qml now.")
        .arg(calls.join(QLatin1Char('\n')));
}

Result<QString> appendNote(const QString& path, const QString& appId, const QString& text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        return Result<QString>::failure(
            QStringLiteral("Could not save the note: %1").arg(file.errorString()));

    const QString entry
        = QStringLiteral("## %1 %2\n%3\n\n")
              .arg(QDateTime::currentDateTime().toString(Qt::ISODate), appId, text.trimmed());
    if (file.write(entry.toUtf8()) < 0)
        return Result<QString>::failure(
            QStringLiteral("Could not save the note: %1").arg(file.errorString()));
    return Result<QString>::success(QStringLiteral("ok"));
}

QString withLayout(const QString& result, const QString& appId)
{
    const AppLayout::Report layout = AppLayout::measureApp(appId);
    if (!layout.fits())
        return QStringLiteral("Saved and loaded, but the layout has problems.\n%1")
            .arg(layout.text());
    if (layout.files.isEmpty())
        return result;
    return QStringLiteral("%1\nLoaded without errors, and the layout of the open screen fits.")
        .arg(result);
}
}

AppTools::AppTools(::Agent& agent, AppRegistry& registry, const QString& notesPath, QObject* parent)
    : QObject(parent)
    , m_agent(agent)
    , m_registry(registry)
    , m_notesPath(notesPath)
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
    m_rejected.clear();
    m_written.clear();
    emit activitiesChanged();
}

void AppTools::registerTools()
{
    addCreateAppTool();
    addListAppsTool();

    addFileTool(kListFilesDocstring,
                { "listing files in", "listed files in", "could not list files in", "app" }, false,
                [](AppFiles& files, const QVariantMap&) { return files.list(); });

    addFileTool(kReadFileDocstring, { "reading", "read", "could not read", "path" }, false,
                [](AppFiles& files, const QVariantMap& arguments)
                {
                    const int from = qMax(1, number(arguments, "from"));
                    return files.read(argument(arguments, "path"), from,
                                      qMax(0, number(arguments, "lines")));
                });

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
    addCheckLayoutTool();
    if (AppScreenshot::enabled())
        addScreenshotTool();
    addGrantAccessTool();
    addSetAppIconTool();
    addNoteTool();
}

void AppTools::addListAppsTool()
{
    m_agent.addTool(
        QString::fromLatin1(kListAppsDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "listing apps", "listed apps", "could not list apps",
                                      "none" };
            const int index = beginActivity(activity, arguments);
            finish(index, Result<QString>::success(appList()), done);
        },
        ToolEffect::Reads);
}

void AppTools::addNoteTool()
{
    m_agent.addTool(
        QString::fromLatin1(kNoteDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "noting", "noted", "could not note", "app" };
            const int index = beginActivity(activity, arguments);
            QString note;
            const App app = appFor(arguments, &note);
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            finish(index, appendNote(m_notesPath, app.id(), argument(arguments, "text")), done,
                   note);
        },
        ToolEffect::Reads);
}

void AppTools::addCheckLayoutTool()
{
    m_agent.addTool(
        QString::fromLatin1(kCheckLayoutDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "measuring", "measured", "could not measure", "app" };
            const int index = beginActivity(activity, arguments);
            QString note;
            const App app = appFor(arguments, &note);
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            if (!AppLoader::running(app.id()))
            {
                finish(index,
                       Result<QString>::failure(
                           QStringLiteral("\"%1\" is not on screen, so it cannot be measured")
                               .arg(app.id())),
                       done, note);
                return;
            }

            const AppScreen view(app.id(), argument(arguments, "screen"));
            if (!view.error().isEmpty())
            {
                finish(index, Result<QString>::failure(view.error()), done, note);
                return;
            }

            finish(index, Result<QString>::success(AppLayout::measureApp(app.id()).text()), done,
                   note);
        },
        ToolEffect::Reads);
}

void AppTools::addScreenshotTool()
{
    m_agent.addTool(
        QString::fromLatin1(kScreenshotDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "photographing", "photographed", "could not photograph",
                                      "app" };
            const int index = beginActivity(activity, arguments);
            QString note;
            const App app = appFor(arguments, &note);
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            const AppScreen view(app.id(), argument(arguments, "screen"));
            if (!view.error().isEmpty())
            {
                finish(index, Result<QString>::failure(view.error()), done, note);
                return;
            }

            Result<ToolImage> shot = AppScreenshot::capture(app.id());
            if (shot.isFailure())
            {
                finish(index, Result<QString>::failure(shot.error()), done, note);
                return;
            }

            ToolImage image = shot.value();
            image.text = note + image.text;
            endActivity(index, true);
            done(QVariant::fromValue(image));
        },
        ToolEffect::Reads);
}

void AppTools::addDescribeAppTool()
{
    m_agent.addTool(
        QString::fromLatin1(kDescribeAppDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "inspecting", "inspected", "could not inspect", "app" };
            const int index = beginActivity(activity, arguments);
            QString note;
            const App app = appFor(arguments, &note);
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            const Result<QString> files = AppFiles(app.dir()).outline();
            if (files.isFailure())
            {
                finish(index, files, done, note);
                return;
            }

            finish(index,
                   Result<QString>::success(files.value() + QStringLiteral("\n\n")
                                            + AppSchema(app).describe()),
                   done, note);
        },
        ToolEffect::Reads);
}

void AppTools::addGrantAccessTool()
{
    m_agent.addTool(
        QString::fromLatin1(kGrantAccessDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "sharing", "shared", "could not share", "store" };
            const int index = beginActivity(activity, arguments);
            QString note;
            const App app = appFor(arguments, &note);
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

            show(app,
                 note + QStringLiteral("\"%1\" may use %2 now").arg(app.id(), storeObject(store)),
                 index, done);
        },
        ToolEffect::Writes);
}

void AppTools::addSetAppIconTool()
{
    m_agent.addTool(
        QString::fromLatin1(kSetAppIconDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "choosing icon for", "chose icon for",
                                      "could not choose icon for", "app" };
            const int index = beginActivity(activity, arguments);
            QString note;
            const App app = appFor(arguments, &note);
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            const Result<QString> icon = iconNamed(arguments);
            if (icon.isFailure())
            {
                finish(index, icon, done);
                return;
            }

            if (!m_registry.setIcon(app.id(), icon.value()))
            {
                finish(index,
                       Result<QString>::failure(
                           QStringLiteral("Could not change the icon of \"%1\"").arg(app.id())),
                       done);
                return;
            }

            finish(index,
                   Result<QString>::success(
                       QStringLiteral("\"%1\" has the icon %2 now").arg(app.id(), icon.value())),
                   done, note);
        },
        ToolEffect::Writes);
}

void AppTools::addDeleteAppTool()
{
    m_agent.addTool(
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
        },
        ToolEffect::Writes);
}

void AppTools::addCreateAppTool()
{
    m_agent.addTool(
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

            const Result<QString> icon = iconNamed(arguments);
            if (icon.isFailure())
            {
                finish(index, icon, done);
                return;
            }

            AppFiles files(app.dir());
            const AppSources previous = files.snapshot();
            const QString priorError = currentError(app);
            const Result<QString> written
                = files.write(QStringLiteral("Main.qml"), argument(arguments, "qml"));
            if (written.isFailure())
            {
                finish(index, written, done);
                return;
            }

            const QString data = argument(arguments, "data");
            const Result<QString> dataWritten = data.trimmed().isEmpty()
                ? Result<QString>::success(QString())
                : files.write(QStringLiteral("Data.qml"), data);
            if (dataWritten.isFailure())
            {
                finish(index, dataWritten, done);
                return;
            }

            m_registry.record(id, argument(arguments, "title"), icon.value());
            show(app, QStringLiteral("ok"), index, done, previous, priorError);
        },
        ToolEffect::Writes);
}

void AppTools::addReadLogTool()
{
    m_agent.addTool(
        QString::fromLatin1(kReadLogDocstring),
        [this](QVariantMap arguments, ToolDone done)
        {
            const Activity activity { "checking", "checked", "could not check", "app" };
            const int index = beginActivity(activity, arguments);
            QString note;
            const App app = appFor(arguments, &note);
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            const QStringList lines = m_warnings ? m_warnings->lines(app.id()) : QStringList();
            const QString pending = unfinished(app);
            const QString text
                = pending.isEmpty() ? logText(lines) : logText(lines) + QLatin1Char('\n') + pending;
            finish(index, Result<QString>::success(text), done, note);
        },
        ToolEffect::Reads);
}

void AppTools::addFileTool(const char* docstring, const Activity& activity, bool reloads,
                           const FileOperation& operation)
{
    m_agent.addTool(
        QString::fromLatin1(docstring),
        [this, activity, reloads, operation](QVariantMap arguments, ToolDone done)
        {
            const int index = beginActivity(activity, arguments);
            QString note;
            const App app = appFor(arguments, &note);
            if (const std::optional<QString> refused = refusal(app))
            {
                finish(index, Result<QString>::failure(*refused), done);
                return;
            }

            AppFiles files(app.dir());
            const AppSources previous = files.snapshot();
            const QString priorError = reloads ? currentError(app) : QString();
            const Result<QString> result = operation(files, arguments);
            if (result.isFailure() || !reloads)
            {
                finish(index, result, done, note);
                return;
            }

            const QString key = app.id() + QLatin1Char('/') + argument(arguments, "path");
            show(
                app, note + result.value(), index,
                [this, key, done](const QVariant& answer)
                {
                    noteOutcome(key, answer.toString());
                    done(answer);
                },
                previous, priorError);
        },
        reloads ? ToolEffect::Writes : ToolEffect::Reads);
}

void AppTools::noteOutcome(const QString& key, const QString& answer)
{
    if (answer.startsWith(QStringLiteral("Rejected and undone")))
    {
        m_rejected.insert(key);
        return;
    }
    m_rejected.remove(key);
    m_written.insert(key);
}

QString AppTools::unfinished(const App& app, bool thisTurnOnly) const
{
    QStringList lines;
    QStringList rejected;
    const QString prefix = app.id() + QLatin1Char('/');
    for (const QString& key : m_rejected)
    {
        if (key.startsWith(prefix))
            rejected.append(key.mid(prefix.size()));
    }
    rejected.sort();
    if (!rejected.isEmpty())
        lines.append(QStringLiteral("Rejected earlier this turn and never redone, so still as "
                                    "they were: %1")
                         .arg(rejected.join(QStringLiteral(", "))));

    QStringList orphans = AppFiles(app.dir()).unused();
    if (thisTurnOnly)
    {
        orphans.erase(std::remove_if(orphans.begin(), orphans.end(),
                                     [this, &prefix](const QString& name)
                                     { return !m_written.contains(prefix + name); }),
                      orphans.end());
    }
    if (!orphans.isEmpty())
        lines.append(QStringLiteral("Written, but no other file uses them, so the user cannot "
                                    "reach them: %1")
                         .arg(orphans.join(QStringLiteral(", "))));
    const QStringList calls = AppFiles(app.dir()).undefinedCalls();
    if (!calls.isEmpty())
        lines.append(QStringLiteral("Will throw when that screen opens:\n%1")
                         .arg(calls.join(QLatin1Char('\n'))));
    return lines.join(QLatin1Char('\n'));
}

QString AppTools::runtimeCheck(const App& app) const
{
    if (m_activities.isEmpty() || !app.exists())
        return QString();

    const QString pending = unfinished(app, true);
    if (pending.isEmpty())
        return QString();

    return QStringLiteral("Runtime check — the app is not finished:\n%1\nGo on until read_log "
                          "lists nothing undone, then one sentence.")
        .arg(pending);
}

std::optional<QString> AppTools::refusal(const App& app) const
{
    if (!app.isValid())
        return invalidId(app.id());
    if (!app.exists())
        return unknownApp(app.id());
    return std::nullopt;
}

void AppTools::finish(int index, const Result<QString>& result, const ToolDone& done,
                      const QString& note)
{
    endActivity(index, result.isSuccess());
    done(note + (result.isSuccess() ? result.value() : result.error()));
}

void AppTools::setCurrentApp(const QString& id) { m_currentApp = id; }

App AppTools::appFor(const QVariantMap& arguments, QString* note) const
{
    const App named(argument(arguments, "app"));
    if (named.exists() || m_currentApp.isEmpty() || !App(m_currentApp).exists())
        return named;

    *note = QStringLiteral("No app \"%1\" — the open app is \"%2\", so this went to it:\n")
                .arg(named.id(), m_currentApp);
    return App(m_currentApp);
}

QString AppTools::currentError(const App& app) const
{
    if (!m_engine || !app.exists())
        return QString();
    const Result<void> compiled = AppCompiler(*m_engine).compile(app);
    return compiled.isFailure() ? compiled.error() : QString();
}

void AppTools::show(const App& app, const QString& result, int index, const ToolDone& done,
                    const AppSources& restoreTo, const QString& priorError)
{
    const Result<void> compiled
        = m_engine ? AppCompiler(*m_engine).compile(app) : Result<void>::success();
    m_currentApp = app.id();
    if (m_warnings)
    {
        m_warnings->clear(app.id());
        m_warnings->setCurrentApp(app.id());
    }

    if (compiled.isFailure() && !restoreTo.isEmpty()
        && !onlyOldErrors(compiled.error(), priorError))
    {
        AppFiles(app.dir()).restore(restoreTo);
        emit appShown(app.id());
        finish(index,
               Result<QString>::failure(wasRolledBack(explained(compiled.error(), app.dir()))),
               done);
        return;
    }

    emit appShown(app.id());

    if (compiled.isFailure())
    {
        endActivity(index, !priorError.isEmpty() && onlyOldErrors(compiled.error(), priorError));
        done(doesNotLoad(explained(compiled.error(), app.dir())));
        return;
    }

    if (!m_warnings)
    {
        finish(index, Result<QString>::success(result), done);
        return;
    }

    QTimer::singleShot(
        kSettleMs, this,
        [this, id = app.id(), result, index, done]
        {
            const QStringList raised = m_warnings->lines(id);
            if (!raised.isEmpty())
            {
                finish(index, Result<QString>::failure(raisedWhileLoading(raised)), done);
                return;
            }
            const QStringList calls = AppFiles(App(id).dir()).undefinedCalls();
            if (!calls.isEmpty())
            {
                finish(index, Result<QString>::success(missingFunctions(calls)), done);
                return;
            }
            finish(index, Result<QString>::success(withLayout(result, id)), done);
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

QString AppTools::appList() const
{
    QStringList lines;
    const QVariantList apps = m_registry.apps();
    for (const QVariant& app : apps)
    {
        const QVariantMap entry = app.toMap();
        lines.append(QStringLiteral("%1  %2").arg(entry.value(QStringLiteral("id")).toString(),
                                                  entry.value(QStringLiteral("title")).toString()));
    }

    if (lines.isEmpty())
        return QStringLiteral("There are no apps yet");
    return lines.join(QLatin1Char('\n'));
}

Result<QString> AppTools::iconNamed(const QVariantMap& arguments) const
{
    const AppIcons icons(m_engine);
    const QString name = argument(arguments, "icon");
    const QString resolved = icons.resolve(name);
    if (resolved.isEmpty())
        return Result<QString>::failure(QStringLiteral("No icon \"%1\": choose one of %2")
                                            .arg(name, icons.names().join(QLatin1Char(' '))));
    return Result<QString>::success(resolved);
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
