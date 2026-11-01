#include "runtime/host/applayout.h"

#include "runtime/app/app.h"
#include "runtime/host/apploader.h"

#include <QFileInfo>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QStringList>
#include <QUrl>

namespace
{
const int kMaxLines = 8;
const int kLabelLength = 24;
const qreal kSlack = 1.0;

struct Walk
{
    QQuickItem* root;
    QString dir;
    QStringList lines;
    int hidden = 0;
};

QString typeName(const QObject* object)
{
    QString name = QString::fromLatin1(object->metaObject()->className());
    for (const char* marker : { "_QMLTYPE", "_QML_" })
    {
        const int cut = name.indexOf(QLatin1String(marker));
        if (cut > 0)
            name.truncate(cut);
    }
    if (name.startsWith(QStringLiteral("QQuick")))
        name.remove(0, 6);
    return name;
}

QString fileOf(const QObject* object)
{
    const QQmlContext* context = QQmlEngine::contextForObject(object);
    return context ? QFileInfo(context->baseUrl().toLocalFile()).fileName() : QString();
}

bool ownedBy(const QObject* object, const QString& dir)
{
    const QQmlContext* context = QQmlEngine::contextForObject(object);
    return context && context->baseUrl().toLocalFile().startsWith(dir + QLatin1Char('/'));
}

QString label(QQuickItem* item)
{
    QString text;
    for (const char* property : { "text", "title", "label", "placeholder" })
    {
        text = item->property(property).toString().simplified();
        if (!text.isEmpty())
            break;
    }
    if (text.size() > kLabelLength)
        text = text.left(kLabelLength - 1) + QChar(0x2026);

    QString name = typeName(item);
    if (!text.isEmpty())
        name += QStringLiteral(" \"%1\"").arg(text);
    return name + QStringLiteral(" (%1)").arg(fileOf(item));
}

QString describe(QQuickItem* item, const Walk& walk)
{
    for (QQuickItem* candidate = item; candidate; candidate = candidate->parentItem())
    {
        if (candidate == walk.root)
            return QStringLiteral("the page");
        if (ownedBy(candidate, walk.dir))
            return label(candidate);
    }
    return QStringLiteral("the page");
}

void add(Walk& walk, const QString& line)
{
    if (walk.lines.size() < kMaxLines)
        walk.lines.append(line);
    else
        ++walk.hidden;
}

bool isText(const QQuickItem* item) { return item->inherits("QQuickText"); }

bool isLayout(const QQuickItem* item) { return item->inherits("QQuickLayout"); }

bool isFlickable(const QQuickItem* item) { return item->inherits("QQuickFlickable"); }

bool hasVisibleChildren(const QQuickItem* item)
{
    const QList<QQuickItem*> children = item->childItems();
    for (const QQuickItem* child : children)
    {
        if (child->isVisible() && !child->inherits("QQuickRepeater"))
            return true;
    }
    return false;
}

bool showsSomething(const QQuickItem* item)
{
    return (item->flags() & QQuickItem::ItemHasContents) || hasVisibleChildren(item);
}

qreal childrenRight(const QQuickItem* item)
{
    qreal right = 0;
    const QList<QQuickItem*> children = item->childItems();
    for (const QQuickItem* child : children)
    {
        if (child->isVisible() && child->width() > 0)
            right = qMax(right, child->x() + child->width());
    }
    return right;
}

qreal childrenBottom(const QQuickItem* item)
{
    qreal bottom = 0;
    const QList<QQuickItem*> children = item->childItems();
    for (const QQuickItem* child : children)
    {
        if (child->isVisible() && child->height() > 0)
            bottom = qMax(bottom, child->y() + child->height());
    }
    return bottom;
}

bool tooNarrow(const QQuickItem* item) { return childrenRight(item) > item->width() + kSlack; }

bool tooShort(const QQuickItem* item) { return childrenBottom(item) > item->height() + kSlack; }

QQuickItem* widestChild(QQuickItem* item)
{
    QQuickItem* widest = nullptr;
    const QList<QQuickItem*> children = item->childItems();
    for (QQuickItem* child : children)
    {
        if (child->isVisible() && (!widest || child->width() > widest->width()))
            widest = child;
    }
    return widest;
}

bool aChildLayoutIs(QQuickItem* item, bool (*failing)(const QQuickItem*))
{
    const QList<QQuickItem*> children = item->childItems();
    for (QQuickItem* child : children)
    {
        if (child->isVisible() && isLayout(child) && failing(child))
            return true;
    }
    return false;
}

void checkLayout(QQuickItem* item, Walk& walk, bool inScroll, bool& heightDone, bool& widthDone)
{
    if (item->width() < kSlack || item->height() < kSlack)
        return;

    if (!heightDone && !inScroll && tooShort(item) && !aChildLayoutIs(item, tooShort))
    {
        heightDone = true;
        add(walk,
            QStringLiteral("%1 needs %2px of height but has %3px: the bottom is cut off (set "
                           "scrollable: true on the page, or let one child shrink with "
                           "Layout.fillHeight: true)")
                .arg(describe(item, walk))
                .arg(qRound(childrenBottom(item)))
                .arg(qRound(item->height())));
    }
    if (!widthDone && tooNarrow(item) && !aChildLayoutIs(item, tooNarrow))
    {
        widthDone = true;
        QString line = QStringLiteral("%1 needs %2px of width but has %3px: its children do not "
                                      "fit side by side")
                           .arg(describe(item, walk))
                           .arg(qRound(childrenRight(item)))
                           .arg(qRound(item->width()));
        if (QQuickItem* widest = widestChild(item))
            line += QStringLiteral("; the widest is %1 at %2px")
                        .arg(describe(widest, walk))
                        .arg(qRound(widest->width()));
        add(walk,
            line
                + QStringLiteral(" (give every child Layout.fillWidth: true, or put fewer "
                                 "things in the row)"));
    }
}

void checkOwned(QQuickItem* item, Walk& walk, bool inScroll, bool heightDone, bool& widthDone)
{
    const QRectF rect = item->mapRectToItem(walk.root, QRectF(0, 0, item->width(), item->height()));
    const QString name = label(item);

    if (!widthDone && rect.right() > walk.root->width() + kSlack)
    {
        widthDone = true;
        add(walk,
            QStringLiteral("%1 sticks out of the screen on the right by %2px")
                .arg(name)
                .arg(qRound(rect.right() - walk.root->width())));
    }
    if (!heightDone && !inScroll && rect.bottom() > walk.root->height() + kSlack
        && rect.top() < walk.root->height())
    {
        add(walk,
            QStringLiteral("%1 runs past the bottom of the screen by %2px")
                .arg(name)
                .arg(qRound(rect.bottom() - walk.root->height())));
    }

    const bool emptyText = isText(item) && item->property("text").toString().isEmpty();
    const QVariant count = item->property("count");
    if (count.isValid() && count.toInt() == 0)
        return;
    if (!showsSomething(item))
        return;
    if (item->height() < kSlack && !emptyText)
        add(walk, QStringLiteral("%1 has no height, so it is invisible").arg(name));
    else if (item->width() < kSlack && !emptyText)
        add(walk, QStringLiteral("%1 has no width, so it is invisible").arg(name));

    if (isText(item) && !emptyText)
    {
        if (item->property("truncated").toBool())
            add(walk, QStringLiteral("%1 is cut off: the text does not fit").arg(name));
        else if (item->property("contentWidth").toReal() > item->width() + kSlack)
            add(walk,
                QStringLiteral("%1 is wider than its space by %2px")
                    .arg(name)
                    .arg(qRound(item->property("contentWidth").toReal() - item->width())));
    }
}

void walkItem(QQuickItem* item, Walk& walk, bool inScroll, bool heightDone, bool widthDone)
{
    if (!item->isVisible() || item->opacity() <= 0.0)
        return;

    item->ensurePolished();

    if (isLayout(item))
        checkLayout(item, walk, inScroll, heightDone, widthDone);

    if (item != walk.root && !item->inherits("QQuickRepeater") && ownedBy(item, walk.dir))
        checkOwned(item, walk, inScroll, heightDone, widthDone);

    if (isFlickable(item) && item->property("interactive").toBool())
        inScroll = true;

    const QList<QQuickItem*> children = item->childItems();
    for (QQuickItem* child : children)
        walkItem(child, walk, inScroll, heightDone, widthDone);
}
}

QString AppLayout::report(QQuickItem* root, const QString& appDir)
{
    if (!root)
        return QString();

    Walk walk { root, appDir, QStringList(), 0 };
    walkItem(root, walk, false, false, false);
    if (walk.lines.isEmpty())
        return QString();

    QString text = QStringLiteral("Layout problems on a %1x%2 screen:\n- %3")
                       .arg(qRound(root->width()))
                       .arg(qRound(root->height()))
                       .arg(walk.lines.join(QStringLiteral("\n- ")));
    if (walk.hidden > 0)
        text += QStringLiteral("\n- and %1 more").arg(walk.hidden);
    return text;
}

QString AppLayout::reportFor(const QString& appId)
{
    AppLoader* loader = AppLoader::running(appId);
    if (!loader || !loader->item())
        return QString();
    return report(loader->item(), App(appId).dir());
}
