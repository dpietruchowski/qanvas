#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QStandardPaths>

#include "async/backendworker.h"
#include "platform/keyboardinsetprovider.h"
#include "qmlutils/coloredsvgprovider.h"
#include "qmlutils/qmlregistrator.h"
#include "runtime/app/appregistry.h"
#include "runtime/chat/chathost.h"
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

    QmlRegistrator registrator(engine, QStringLiteral("qrc:/" APP_QML_URI "/"),
                               QStringLiteral(APP_QML_URI));
    registrator.registerSingletonType(QStringLiteral("Themed.Components"),
                                      QStringLiteral("Components/Theme.qml"),
                                      QStringLiteral("Theme"));
    registrator.registerSingletonInstance("Themed.Components", "KeyboardInset", &keyboardInset);
    registrator.registerType<ColoredSvgProvider>("Themed.Components", "ColoredSvgProvider");
    registrator.registerType<AppLoader>("AppLoader");
    qmlRegisterSingletonType<AppRegistry>(APP_QML_URI, 1, 0, "AppRegistry", &AppRegistry::create);

    BackendWorker worker;
    ChatHost chatHost(ChatHost::Config { .dataDirectory = QStandardPaths::writableLocation(
                                             QStandardPaths::AppDataLocation),
                                         .settingsOrganization = app.organizationName(),
                                         .settingsApplication = app.applicationName() },
                      engine, worker, AppRegistry::instance());
    chatHost.registerQml(registrator);

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
