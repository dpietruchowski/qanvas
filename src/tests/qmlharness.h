#pragma once

#include <gtest/gtest.h>

#include <memory>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>

#include "themedtypes.h"

class QmlHarness : public ::testing::Test
{
protected:
    void SetUp() override
    {
        registerThemedTypes();
        engine.addImportPath(QStringLiteral(QANVAS_QML_IMPORTS));
        engine.setOutputWarningsToStandardError(false);
        window.resize(400, 800);
        window.show();
        ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
    }

    QQuickItem* show(const QString& body)
    {
        const QString source = QStringLiteral("import QtQuick\nimport QtQuick.Layouts\n"
                                              "import Qanvas.Components\n")
                               + body;
        QQmlComponent component(&engine);
        component.setData(source.toUtf8(), QUrl());
        if (component.isError())
        {
            ADD_FAILURE() << component.errorString().toStdString();
            return nullptr;
        }
        std::unique_ptr<QObject> object(component.create());
        if (!object)
        {
            ADD_FAILURE() << component.errorString().toStdString();
            return nullptr;
        }
        auto* item = qobject_cast<QQuickItem*>(object.get());
        if (!item)
        {
            ADD_FAILURE() << "root is not an Item";
            return nullptr;
        }
        object.release();
        owned.reset(item);
        item->setParentItem(window.contentItem());
        QTest::qWait(30);
        return item;
    }

    QQmlEngine engine;
    QQuickWindow window;
    std::unique_ptr<QQuickItem> owned;
};
