#include <gtest/gtest.h>

#include <memory>

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QVariantMap>

#include "runtime/app/appregistry.h"
#include "runtime/host/notesbridge.h"
#include "runtime/storage/notesstore.h"

class NotesBridgeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit());
        registry = std::make_unique<AppRegistry>(nullptr);
    }

    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
    std::unique_ptr<AppRegistry> registry;
};

TEST_F(NotesBridgeTest, RefusesEverythingWithoutTheCapability)
{
    NotesStore().add(QStringLiteral("secret"));
    NotesBridge bridge(QStringLiteral("spy"), *registry);
    int changes = 0;
    QObject::connect(&bridge, &NotesBridge::changed, [&changes] { ++changes; });

    EXPECT_FALSE(bridge.allowed());
    EXPECT_TRUE(bridge.list().isEmpty());
    EXPECT_FALSE(bridge.add(QStringLiteral("mine")));
    EXPECT_FALSE(bridge.update(1, QStringLiteral("mine")));
    EXPECT_FALSE(bridge.remove(1));
    EXPECT_EQ(bridge.lastError(), QStringLiteral("\"spy\" may not use the user's notes"));
    EXPECT_EQ(bridge.revision(), 0);
    EXPECT_EQ(changes, 0);
    EXPECT_EQ(NotesStore().list().size(), 1);
}

TEST_F(NotesBridgeTest, ReadsAndWritesOnceGranted)
{
    registry->grant(QStringLiteral("notes"), NotesBridge::capability());
    NotesBridge bridge(QStringLiteral("notes"), *registry);
    int changes = 0;
    QObject::connect(&bridge, &NotesBridge::changed, [&changes] { ++changes; });

    EXPECT_TRUE(bridge.allowed());
    EXPECT_TRUE(bridge.add(QStringLiteral("milk")));
    ASSERT_EQ(bridge.list().size(), 1);
    const int id = bridge.list().first().toMap().value(QStringLiteral("id")).toInt();
    EXPECT_TRUE(bridge.update(id, QStringLiteral("oat milk")));
    EXPECT_EQ(bridge.list().first().toMap().value(QStringLiteral("text")).toString(),
              QStringLiteral("oat milk"));
    EXPECT_TRUE(bridge.remove(id));
    EXPECT_TRUE(bridge.list().isEmpty());
    EXPECT_EQ(bridge.revision(), 3);
    EXPECT_EQ(changes, 3);
    EXPECT_TRUE(bridge.lastError().isEmpty());
}

TEST_F(NotesBridgeTest, AFailedWriteExplainsItselfAndBumpsNothing)
{
    registry->grant(QStringLiteral("notes"), NotesBridge::capability());
    NotesBridge bridge(QStringLiteral("notes"), *registry);

    EXPECT_FALSE(bridge.add(QStringLiteral("  ")));
    EXPECT_EQ(bridge.lastError(), QStringLiteral("A note needs some text"));
    EXPECT_FALSE(bridge.remove(42));
    EXPECT_EQ(bridge.lastError(), QStringLiteral("No note 42 to remove"));
    EXPECT_EQ(bridge.revision(), 0);
}

TEST_F(NotesBridgeTest, AppsShareOneSetOfNotes)
{
    registry->grant(QStringLiteral("writer"), NotesBridge::capability());
    registry->grant(QStringLiteral("reader"), NotesBridge::capability());
    NotesBridge writer(QStringLiteral("writer"), *registry);
    NotesBridge reader(QStringLiteral("reader"), *registry);

    writer.add(QStringLiteral("milk"));

    ASSERT_EQ(reader.list().size(), 1);
    EXPECT_EQ(reader.list().first().toMap().value(QStringLiteral("text")).toString(),
              QStringLiteral("milk"));
}

TEST_F(NotesBridgeTest, QmlReachesItAsAContextProperty)
{
    registry->grant(QStringLiteral("notes"), NotesBridge::capability());
    NotesBridge bridge(QStringLiteral("notes"), *registry);
    QQmlEngine engine;
    QQmlContext context(engine.rootContext());
    context.setContextProperty(QStringLiteral("Notes"), &bridge);
    QQmlComponent component(&engine);
    component.setData("import QtQuick\n"
                      "QtObject {\n"
                      "    property int count: (Notes.revision, Notes.list().length)\n"
                      "    property bool allowed: Notes.allowed\n"
                      "    Component.onCompleted: Notes.add(\"from qml\")\n"
                      "}\n",
                      QUrl());

    std::unique_ptr<QObject> object(component.create(&context));

    ASSERT_TRUE(object) << component.errorString().toStdString();
    EXPECT_TRUE(object->property("allowed").toBool());
    EXPECT_EQ(object->property("count").toInt(), 1);
    EXPECT_EQ(NotesStore().list().size(), 1);
}
