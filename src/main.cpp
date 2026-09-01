#include "apploader.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

namespace
{

// Generated apps live on disk, outside the resource system, so the agent can
// rewrite them between runs. Seeded once with a placeholder so there is
// something to load on a fresh install.
void seedDemoApp()
{
    const QString dir = AppLoader::appsRoot() + QStringLiteral("/hello");
    const QString entry = dir + QStringLiteral("/Main.qml");
    if (QFile::exists(entry) || !QDir().mkpath(dir))
        return;

    QFile file(entry);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    file.write(R"QML(import QtQuick

Rectangle {
    color: "#1b2430"

    Component.onCompleted: App.exec(
        "CREATE TABLE IF NOT EXISTS notes (id INTEGER PRIMARY KEY, text TEXT)")

    Column {
        anchors.centerIn: parent
        spacing: 12

        Text {
            text: "Edit this file, then press Run app"
            color: "#e6edf3"
            font.pixelSize: 18
        }

        Text {
            // Re-runs whenever the app writes, so the view never goes stale.
            property var rows: (App.revision, App.query("SELECT count(*) AS n FROM notes"))
            text: "rows in notes: " + (rows.length ? rows[0].n : "?")
            color: "#8b98a5"
            font.pixelSize: 14
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: App.exec("INSERT INTO notes (text) VALUES (?)", ["tap"])
    }
}
)QML");
}

}  // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Qanvas"));
    app.setOrganizationName(QStringLiteral("Qanvas"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));

    seedDemoApp();

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/Qanvas/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
