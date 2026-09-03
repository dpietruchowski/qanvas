#include "runtime/app/app.h"

#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

namespace
{
const QRegularExpression& idPattern()
{
    static const QRegularExpression pattern(QStringLiteral("^[a-z0-9][a-z0-9-]{0,31}$"));
    return pattern;
}
}

QString App::root()
{
    const QByteArray override = qgetenv("QANVAS_APPS_ROOT");
    if (!override.isEmpty())
        return QDir::cleanPath(QString::fromLocal8Bit(override));

#ifdef Q_OS_ANDROID
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/apps");
#else
    return QCoreApplication::applicationDirPath() + QStringLiteral("/apps");
#endif
}

bool App::isValidId(const QString& id) { return idPattern().match(id).hasMatch(); }

App::App(QString id)
    : m_id(std::move(id))
{
}

bool App::isValid() const { return isValidId(m_id); }

bool App::exists() const { return isValid() && QFileInfo::exists(entryPath()); }

QString App::dir() const
{
    if (!isValid())
        return QString();
    return root() + QLatin1Char('/') + m_id;
}

QString App::entryPath() const { return child(QStringLiteral("Main.qml")); }

QString App::databasePath() const { return child(QStringLiteral("app.db")); }

QString App::iconPath() const { return child(QStringLiteral("icon.svg")); }

QString App::child(const QString& name) const
{
    const QString directory = dir();
    if (directory.isEmpty())
        return QString();
    return directory + QLatin1Char('/') + name;
}
