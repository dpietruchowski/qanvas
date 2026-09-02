#include "appcompiler.h"

#include "app.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QStringList>
#include <QUrl>

AppCompiler::AppCompiler(QQmlEngine& engine)
    : m_engine(engine)
{
}

Result<void> AppCompiler::compile(const App& app) const
{
    if (!app.exists())
        return Result<void>::failure(QStringLiteral("No app \"%1\"").arg(app.id()));

    m_engine.clearComponentCache();

    const QQmlComponent component(&m_engine, QUrl::fromLocalFile(app.entryPath()));
    if (!component.isError())
        return Result<void>::success();

    const QString directory = QUrl::fromLocalFile(app.dir()).toString() + QLatin1Char('/');
    QStringList lines;
    const QList<QQmlError> errors = component.errors();
    for (const QQmlError& error : errors)
        lines.append(error.toString().remove(directory));

    return Result<void>::failure(lines.join(QLatin1Char('\n')));
}
