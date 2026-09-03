#include <gtest/gtest.h>

#include <QFile>
#include <QStringList>
#include <QTemporaryDir>
#include <QVariantMap>

#include "runtime/notesstore.h"

namespace
{
QStringList texts(const QVariantList& notes)
{
    QStringList out;
    for (const QVariant& note : notes)
        out.append(note.toMap().value(QStringLiteral("text")).toString());
    return out;
}
}

class NotesStoreTest : public ::testing::Test
{
protected:
    void SetUp() override { qputenv("QANVAS_APPS_ROOT", root.path().toLocal8Bit()); }
    void TearDown() override { qunsetenv("QANVAS_APPS_ROOT"); }

    QTemporaryDir root;
};

TEST_F(NotesStoreTest, KeepsTheNotesInTheSystemDatabase)
{
    NotesStore store;

    EXPECT_TRUE(store.isOpen());
    EXPECT_TRUE(QFile::exists(root.path() + QStringLiteral("/system.db")));
}

TEST_F(NotesStoreTest, AddsNotesAndListsTheNewestFirst)
{
    NotesStore store;

    const qint64 first = store.add(QStringLiteral("milk"));
    const qint64 second = store.add(QStringLiteral("  bread  "));

    EXPECT_GT(first, 0);
    EXPECT_GT(second, first);
    EXPECT_EQ(texts(store.list()),
              (QStringList { QStringLiteral("bread"), QStringLiteral("milk") }));
    const QVariantMap note = store.list().first().toMap();
    EXPECT_EQ(note.value(QStringLiteral("id")).toLongLong(), second);
    EXPECT_FALSE(note.value(QStringLiteral("createdAt")).toString().isEmpty());
    EXPECT_FALSE(note.value(QStringLiteral("updatedAt")).toString().isEmpty());
}

TEST_F(NotesStoreTest, RefusesABlankNote)
{
    NotesStore store;

    EXPECT_EQ(store.add(QStringLiteral("   ")), -1);
    EXPECT_TRUE(store.list().isEmpty());
}

TEST_F(NotesStoreTest, UpdatesAndRemovesByIdOnly)
{
    NotesStore store;
    const qint64 id = store.add(QStringLiteral("milk"));

    EXPECT_TRUE(store.update(id, QStringLiteral("oat milk")));
    EXPECT_FALSE(store.update(id, QStringLiteral(" ")));
    EXPECT_FALSE(store.update(id + 1, QStringLiteral("nothing")));
    EXPECT_EQ(texts(store.list()), QStringList { QStringLiteral("oat milk") });

    EXPECT_FALSE(store.remove(id + 1));
    EXPECT_TRUE(store.remove(id));
    EXPECT_TRUE(store.list().isEmpty());
}

TEST_F(NotesStoreTest, SurvivesAcrossInstances)
{
    NotesStore().add(QStringLiteral("milk"));

    EXPECT_EQ(texts(NotesStore().list()), QStringList { QStringLiteral("milk") });
}
