#include "runtime/app/appwarnings.h"

#include "runtime/app/app.h"

#include <QFileInfo>
#include <QQmlEngine>
#include <QQmlError>
#include <QUrl>

namespace
{
const int kMaxLinesPerApp = 50;

QString appIdOf(const QUrl& url)
{
    const QString root = App::root() + QLatin1Char('/');
    const QString path = url.toLocalFile();
    if (!path.startsWith(root))
        return QString();

    const QString id = path.mid(root.size()).section(QLatin1Char('/'), 0, 0);
    return App::isValidId(id) ? id : QString();
}
}

AppWarnings::AppWarnings(QQmlEngine& engine, QObject* parent)
    : QObject(parent)
{
    connect(&engine, &QQmlEngine::warnings, this, &AppWarnings::record);
}

void AppWarnings::clear(const QString& appId) { m_lines.remove(appId); }

void AppWarnings::setCurrentApp(const QString& appId) { m_currentApp = appId; }

void AppWarnings::record(const QList<QQmlError>& errors)
{
    for (const QQmlError& error : errors)
    {
        QString id = appIdOf(error.url());
        QString line;
        if (!id.isEmpty())
        {
            const QString directory
                = QUrl::fromLocalFile(App(id).dir()).toString() + QLatin1Char('/');
            line = error.toString().remove(directory);
        }
        else if (!m_currentApp.isEmpty())
        {
            id = m_currentApp;
            line = QStringLiteral("%1:%2: %3")
                       .arg(QFileInfo(error.url().path()).fileName())
                       .arg(error.line())
                       .arg(error.description());
        }
        else
        {
            continue;
        }

        QStringList& lines = m_lines[id];
        lines.append(line);
        if (lines.size() > kMaxLinesPerApp)
            lines.removeFirst();
    }
}
