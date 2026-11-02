#include "runtime/app/appicons.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QUrl>
#include <QVariant>

namespace
{
const char* kThemeReader = "import QtQml\n"
                           "import Qanvas.Components\n"
                           "QtObject {\n"
                           "    readonly property list<string> names: Theme.iconNames\n"
                           "    function resolve(name) { return Theme.resolveIcon(name) }\n"
                           "}\n";
}

AppIcons::AppIcons(QQmlEngine* engine)
{
    if (!engine)
        return;

    QQmlComponent component(engine);
    component.setData(kThemeReader, QUrl());
    m_theme.reset(component.create());
}

QString AppIcons::resolve(const QString& name) const
{
    if (!m_theme)
        return QString();

    QVariant resolved;
    QMetaObject::invokeMethod(m_theme.get(), "resolve", Q_RETURN_ARG(QVariant, resolved),
                              Q_ARG(QVariant, name));
    return resolved.toString();
}

QStringList AppIcons::names() const
{
    return m_theme ? m_theme->property("names").toStringList() : QStringList();
}
