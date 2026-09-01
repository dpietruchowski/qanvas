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
    // Order matters. The item tree goes first so that nothing still references
    // the app's QML files, then the context and the bridge, and only then is the
    // cache trimmed — trimComponentCache() drops compiled files that nobody uses
    // any more, and leaves the runtime's own QML (which is in use) alone.
    delete m_item;
    m_item = nullptr;

    delete m_context;
    m_context = nullptr;

    delete m_app;
    m_app = nullptr;

    if (QQmlEngine* engine = qmlEngine(this))
        engine->trimComponentCache();
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
