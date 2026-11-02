#include "runtime/host/appscreen.h"

#include <QMetaObject>
#include <QQuickItem>
#include <QQuickWindow>

#include "runtime/host/apploader.h"

namespace
{
constexpr const char* kProperty = "screen";
}

AppScreen::AppScreen(const QString& appId, const QString& screen)
{
    if (screen.isEmpty())
        return;

    AppLoader* loader = AppLoader::running(appId);
    if (!loader || !loader->item())
        return;

    QQuickItem* root = loader->item();
    if (root->metaObject()->indexOfProperty(kProperty) < 0)
    {
        m_error = QStringLiteral("The page of \"%1\" has no screen property, so there is no other "
                                 "screen to show")
                      .arg(appId);
        return;
    }

    m_root = root;
    m_previous = root->property(kProperty);
    m_changed = root->setProperty(kProperty, screen);
    if (QQuickWindow* window = loader->window())
        window->grabWindow();
}

AppScreen::~AppScreen()
{
    if (m_changed && m_root)
        m_root->setProperty(kProperty, m_previous);
}
