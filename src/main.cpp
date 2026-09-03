#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>

#include "platform/keyboardinsetprovider.h"
#include "qmlutils/coloredsvgprovider.h"
#include "qmlutils/qmlregistrator.h"
#include "runtime/app/appregistry.h"
#include "runtime/chat/chatagent.h"
#include "runtime/host/apploader.h"

#ifdef LIBS_AUTOMATION
#include "automation/uiautomationserver.h"
#endif

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral(APP_NAME));
    app.setOrganizationName(QStringLiteral(APP_NAME));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;

    KeyboardInsetProvider keyboardInset;

    QmlRegistrator registrator(engine, QStringLiteral("qrc:/" APP_QML_URI),
                               QStringLiteral(APP_QML_URI));
    registrator.registerSingletonType(QStringLiteral("Themed.Components"),
                                      QStringLiteral("Components/Theme.qml"),
                                      QStringLiteral("Theme"));
    registrator.registerSingletonInstance("Themed.Components", "KeyboardInset", &keyboardInset);
    registrator.registerType<ColoredSvgProvider>("Themed.Components", "ColoredSvgProvider");
    registrator.registerType<AppLoader>("AppLoader");
    registrator.registerType<ChatAgent>("ChatAgent");
    qmlRegisterSingletonType<AppRegistry>(APP_QML_URI, 1, 0, "AppRegistry", &AppRegistry::create);

    engine.load(registrator.getMainQmlUrl());

    if (engine.rootObjects().isEmpty())
        return -1;

#ifdef LIBS_AUTOMATION
    const QByteArray automationPort = qgetenv("APP_AUTOMATION_PORT");
    const quint16 port = automationPort.isEmpty() ? 49300 : automationPort.toUShort();
    new UiAutomationServer(&engine, port, &app);
#endif

    return app.exec();
}
