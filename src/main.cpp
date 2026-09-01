#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Qanvas"));
    app.setOrganizationName(QStringLiteral("Qanvas"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/Qanvas/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
