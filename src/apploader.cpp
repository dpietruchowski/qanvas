#include "apploader.h"

#include "appcontext.h"

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

QString AppLoader::appsRoot()
{
    // Generated apps normally live in writable app data. QANVAS_APPS_ROOT points
    // that elsewhere so a checked-in app (test/) can be loaded as-is, without
    // copying it into the user's data directory first.
    const QByteArray override = qgetenv("QANVAS_APPS_ROOT");
    if (!override.isEmpty())
        return QDir::cleanPath(QString::fromLocal8Bit(override));

    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/apps");
}

QString AppLoader::appDir() const
{
    if (m_appId.isEmpty())
        return QString();
    return appsRoot() + QLatin1Char('/') + m_appId;
}

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

    const QString dir = appDir();
    const QString entry = dir + QStringLiteral("/Main.qml");
    if (!QFileInfo::exists(entry))
    {
        fail(QStringLiteral("No such app: %1").arg(entry));
        return;
    }

    // The app's own bridge lives on a child context, so it is named only inside
    // the app. Anything the runtime puts on the root context stays visible, so
    // the root context is not the place for anything privileged.
    m_app = new AppContext(m_appId, dir, this);
    m_context = new QQmlContext(engine->rootContext(), this);
    m_context->setContextProperty(QStringLiteral("App"), m_app);

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
    // Order matters: the item tree goes first so nothing still references the
    // app's QML files, then the context and the bridge, and only then the cache.
    //
    // The cache call has to be clearComponentCache(), not trimComponentCache().
    // trim() is the appealing one — it only drops what nothing references, so it
    // cannot touch the host's own QML — but measured against an edited file on
    // disk it does not drop the app's entry file at all, and reload() then
    // rebuilds the app from the stale compiled unit. Forcing a GC first does not
    // help either, so something inside the engine keeps the unit referenced past
    // the last object that used it.
    //
    // clear() does pick the edit up. Its documented cost is that bindings in
    // objects already created from the cleared components stop working; measured
    // on Qt 6.4 that does not happen — after a reload the host's bindings still
    // track (appDir and the placeholder both update when appId changes), because
    // live objects hold their own reference to the unit and only the cache entry
    // goes away. Re-check this if the engine is ever upgraded.
    delete m_item;
    m_item = nullptr;

    delete m_context;
    m_context = nullptr;

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
