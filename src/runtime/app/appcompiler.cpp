#include "runtime/app/appcompiler.h"

#include "runtime/app/app.h"
#include "runtime/app/appfiles.h"

#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QStringList>
#include <QUrl>

namespace
{
QStringList entryFirst(const App& app)
{
    const QString entry = QFileInfo(app.entryPath()).fileName();
    QStringList names;
    const AppSources sources = AppFiles(app.dir()).snapshot();
    for (auto it = sources.cbegin(); it != sources.cend(); ++it)
    {
        if (it.key().endsWith(QStringLiteral(".qml")) && it.key() != entry)
            names.append(it.key());
    }
    names.prepend(entry);
    return names;
}
}

AppCompiler::AppCompiler(QQmlEngine& engine)
    : m_engine(engine)
{
}

Result<void> AppCompiler::compile(const App& app) const
{
    if (!app.exists())
        return Result<void>::failure(QStringLiteral("No app \"%1\"").arg(app.id()));

    m_engine.clearComponentCache();

    const QString directory = QUrl::fromLocalFile(app.dir()).toString() + QLatin1Char('/');
    QStringList lines;
    const QStringList names = entryFirst(app);
    for (const QString& name : names)
    {
        const QQmlComponent component(&m_engine,
                                      QUrl::fromLocalFile(app.dir() + QLatin1Char('/') + name));
        if (!component.isError())
            continue;

        const QList<QQmlError> errors = component.errors();
        for (const QQmlError& error : errors)
            lines.append(error.toString().remove(directory));
    }

    if (lines.isEmpty())
        return Result<void>::success();

    return Result<void>::failure(lines.join(QLatin1Char('\n')));
}
