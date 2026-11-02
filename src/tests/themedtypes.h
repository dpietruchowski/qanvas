#pragma once

#include <QQmlEngine>
#include <QUrl>

#include "qmlutils/coloredsvgprovider.h"

inline void registerThemedTypes()
{
    static const bool registered = []
    {
        qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/Qanvas/Components/Theme.qml")),
                                 "Themed.Components", 1, 0, "Theme");
        qmlRegisterType<ColoredSvgProvider>("Themed.Components", 1, 0, "ColoredSvgProvider");
        return true;
    }();
    Q_UNUSED(registered);
}
