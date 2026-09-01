#pragma once

#include <QQuickItem>
#include <QString>
#include <QtQml/qqmlregistration.h>

class AppContext;
class QQmlContext;

// Loads one generated app — apps/<id>/Main.qml — as a child item.
//
// This exists instead of a plain QML Loader for one reason: Loader instantiates
// into the context it sits in, and the app needs its own. Every app gets a child
// QQmlContext carrying its own "App" object, so two apps can never see each
// other's bridge, and nothing the runtime keeps in its own QML is named in the
// app's scope.
//
// The app's subtree is rooted here and nowhere else. That is not tidiness: the
// component cache is only trimmed for files nothing references any more, so a
// single item left parented into the runtime's scene would pin the old version
// and the next load would hand back stale QML.
class AppLoader : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString appId READ appId WRITE setAppId NOTIFY appIdChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString appDir READ appDir NOTIFY appIdChanged)

public:
    enum Status
    {
        Null,
        Ready,
        Error
    };
    Q_ENUM(Status)

    explicit AppLoader(QQuickItem* parent = nullptr);
    ~AppLoader() override;

    QString appId() const { return m_appId; }
    void setAppId(const QString& id);

    Status status() const { return m_status; }
    QString errorString() const { return m_errorString; }
    QString appDir() const;

    // Re-reads the app from disk. Setting appId to the same value does nothing,
    // so this is what the runtime calls after the agent rewrites the files.
    Q_INVOKABLE void reload();

    // <writable app data>/apps — the one place generated apps live.
    static QString appsRoot();

signals:
    void appIdChanged();
    void statusChanged();
    void errorStringChanged();

protected:
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    void load();
    void unload();
    void fail(const QString& message);
    void setStatus(Status status);
    void setErrorString(const QString& message);

    QString m_appId;
    Status m_status = Null;
    QString m_errorString;

    QQuickItem* m_item = nullptr;
    QQmlContext* m_context = nullptr;
    AppContext* m_app = nullptr;
};
