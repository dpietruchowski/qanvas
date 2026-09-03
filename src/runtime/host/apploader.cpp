#include "runtime/host/apploader.h"

#include "runtime/app/app.h"
#include "runtime/app/appregistry.h"
#include "runtime/host/appcontext.h"
#include "runtime/host/notesbridge.h"
#include "runtime/host/tasksbridge.h"

#include <QDir>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QUrl>

AppLoader::AppLoader(QQuickItem* parent)
    : QQuickItem(parent)
{
}

AppLoader::~AppLoader() { unload(); }

QString AppLoader::appsRoot() { return App::root(); }

QString AppLoader::appDir() const { return App(m_appId).dir(); }

void AppLoader::setAppId(const QString& id)
{
    if (m_appId == id)
        return;

    m_appId = id;
    emit appIdChanged();
    load();
}

void AppLoader::reload() { load(); }

void AppLoader::load()
{
    unload();

    if (m_appId.isEmpty())
    {
        setStatus(Null);
        return;
    }

    QQmlEngine* engine = qmlEngine(this);
    if (!engine)
    {
        fail(QStringLiteral("AppLoader is not part of a QML engine"));
        return;
    }

    const App app(m_appId);
    const QString dir = app.dir();
    const QString entry = app.entryPath();
    if (!QFileInfo::exists(entry))
    {
        fail(QStringLiteral("No such app: %1").arg(entry));
        return;
    }

    m_app = new AppContext(m_appId, dir, engine, this);
    m_notes = new NotesBridge(m_appId, AppRegistry::instance(), this);
    m_tasks = new TasksBridge(m_appId, AppRegistry::instance(), this);
    m_context = new QQmlContext(engine->rootContext(), this);
    m_context->setContextProperty(QStringLiteral("App"), m_app);
    m_context->setContextProperty(QStringLiteral("Notes"), m_notes);
    m_context->setContextProperty(QStringLiteral("Tasks"), m_tasks);

    QQmlComponent component(engine, QUrl::fromLocalFile(entry));
    if (component.isError())
    {
        fail(component.errorString().trimmed());
        return;
    }

    QObject* object = component.create(m_context);
    if (!object)
    {
        fail(component.errorString().trimmed());
        return;
    }

    m_item = qobject_cast<QQuickItem*>(object);
    if (!m_item)
    {
        delete object;
        fail(QStringLiteral("The app's root object must be an Item"));
        return;
    }

    m_item->setParent(this);
    m_item->setParentItem(this);
    m_item->setSize(size());

    setErrorString(QString());
    setStatus(Ready);
}

void AppLoader::unload()
{
    delete m_item;
    m_item = nullptr;

    delete m_context;
    m_context = nullptr;

    delete m_tasks;
    m_tasks = nullptr;

    delete m_notes;
    m_notes = nullptr;

    delete m_app;
    m_app = nullptr;

    if (QQmlEngine* engine = qmlEngine(this))
        engine->clearComponentCache();
}

void AppLoader::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (m_item)
        m_item->setSize(newGeometry.size());
}

void AppLoader::fail(const QString& message)
{
    unload();
    setErrorString(message);
    setStatus(Error);
}

void AppLoader::setStatus(Status status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void AppLoader::setErrorString(const QString& message)
{
    if (m_errorString == message)
        return;
    m_errorString = message;
    emit errorStringChanged();
}
