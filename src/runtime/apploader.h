#pragma once

#include <QQuickItem>
#include <QString>
#include <QtQml/qqmlregistration.h>

class AppContext;
class NotesBridge;
class QQmlContext;

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

    Q_INVOKABLE void reload();

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
    NotesBridge* m_notes = nullptr;
};
