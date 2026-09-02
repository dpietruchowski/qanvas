#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include "utils/coloredsvgprovider.h"
#include "utils/keyboardinsetprovider.h"
#include "utils/qmlregistrator.h"

#ifdef QANVAS_AUTOMATION
#include "automation/uiautomationserver.h"
#endif

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Qanvas"));
    app.setOrganizationName(QStringLiteral("Qanvas"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;

    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

    QmlRegistrator registrator(engine, QStringLiteral("qrc:/Qanvas/Components"),
                               QStringLiteral("Qanvas.Components"));

    registrator.registerSingletonType(QStringLiteral("Themed.Components"),
                                      QStringLiteral("Theme.qml"), QStringLiteral("Theme"));
    registrator.registerSingletonType(QStringLiteral("App.Components"), QStringLiteral("Theme.qml"),
                                      QStringLiteral("Theme"));
    registrator.registerType<ColoredSvgProvider>("Themed.Components", "ColoredSvgProvider");

    KeyboardInsetProvider keyboardInset;
    registrator.registerSingletonInstance("App.Components", "KeyboardInset", &keyboardInset);
    registrator.registerSingletonInstance("Themed.Components", "KeyboardInset", &keyboardInset);

    engine.load(registrator.getMainQmlUrl());

    if (engine.rootObjects().isEmpty())
        return -1;

#ifdef QANVAS_AUTOMATION
    const QByteArray automationPort = qgetenv("QANVAS_AUTOMATION_PORT");
    const quint16 port = automationPort.isEmpty() ? 49300 : automationPort.toUShort();
    new UiAutomationServer(&engine, port, &app);
#endif

    return app.exec();
}
