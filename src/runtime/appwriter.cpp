#include "appwriter.h"

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
const QRegularExpression& idPattern()
{
    static const QRegularExpression pattern(QStringLiteral("^[a-z0-9][a-z0-9-]{0,31}$"));
    return pattern;
}

void setError(QString* error, const QString& message)
{
    if (error)
        *error = message;
}
}

QString AppWriter::appsRoot()
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

QString AppWriter::appDir(const QString& id)
{
    if (id.isEmpty())
        return QString();
    return appsRoot() + QLatin1Char('/') + id;
}

bool AppWriter::isValidId(const QString& id) { return idPattern().match(id).hasMatch(); }

QString AppWriter::write(const QString& id, const QString& qml, QString* error)
{
    setError(error, QString());

    if (!isValidId(id))
    {
        setError(error,
                 QStringLiteral("Invalid app id \"%1\": use 1-32 characters of a-z, 0-9 and -, "
                                "starting with a letter or digit")
                     .arg(id));
        return QString();
    }

    if (qml.trimmed().isEmpty())
    {
        setError(error, QStringLiteral("The app is empty"));
        return QString();
    }

    const QString dir = appDir(id);
    if (!QDir().mkpath(dir))
    {
        setError(error, QStringLiteral("Cannot create %1").arg(dir));
        return QString();
    }

    const QString path = dir + QStringLiteral("/Main.qml");
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        setError(error, QStringLiteral("Cannot write %1: %2").arg(path, file.errorString()));
        return QString();
    }

    file.write(qml.toUtf8());
    if (!file.commit())
    {
        setError(error, QStringLiteral("Cannot write %1: %2").arg(path, file.errorString()));
        return QString();
    }

    return path;
}
