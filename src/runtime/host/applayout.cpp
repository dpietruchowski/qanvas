#include "runtime/host/applayout.h"

#include "runtime/app/app.h"
#include "runtime/host/apploader.h"

#include <QFileInfo>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSet>
#include <QStringList>
#include <QUrl>

namespace
{
const int kMaxLines = 8;
const int kLabelLength = 24;
const qreal kSlack = 1.0;
const qreal kTolerance = 3.0;
const int kNoWrap = 0;
const int kMinWrappedLines = 2;
const int kHorizontalFlick = 1;
const int kVerticalFlick = 2;

struct Walk
{
    QQuickItem* root;
    QString dir;
    QStringList lines;
    int hidden = 0;
    QList<int> repeats;
    QSet<QString> files;
};

struct Clip
{
    QQuickItem* item = nullptr;
    QRectF rect;
    bool scrollsX = false;
    bool scrollsY = false;
    bool reported = false;
};

struct Branch
{
    bool inScroll = false;
    bool heightDone = false;
    bool widthDone = false;
    Clip clip;
};

struct Sibling
{
    QQuickItem* item;
    QRectF rect;
};

struct Overlap
{
    qreal depth = 0;
    bool vertical = false;
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

QString shorten(const QString& source)
{
    QString text = source.simplified();
    if (text.size() > kLabelLength)
        text = text.left(kLabelLength - 1) + QChar(0x2026);
    return text;
}

QString named(QQuickItem* item)
{
    QString text;
    for (const char* property : { "text", "title", "label", "placeholder" })
    {
        text = shorten(item->property(property).toString());
        if (!text.isEmpty())
            break;
    }

    QString name = typeName(item);
    if (!text.isEmpty())
        name += QStringLiteral(" \"%1\"").arg(text);
    return name;
}

QString label(QQuickItem* item) { return named(item) + QStringLiteral(" (%1)").arg(fileOf(item)); }

QQuickItem* ownerOf(QQuickItem* item, const Walk& walk)
{
    for (QQuickItem* candidate = item; candidate && candidate != walk.root;
         candidate = candidate->parentItem())
    {
        if (ownedBy(candidate, walk.dir))
            return candidate;
    }
    return nullptr;
}

QString describe(QQuickItem* item, const Walk& walk)
{
    QQuickItem* owner = ownerOf(item, walk);
    return owner ? label(owner) : QStringLiteral("the page");
}

void add(Walk& walk, const QString& line)
{
    const int same = walk.lines.indexOf(line);
    if (same >= 0)
        ++walk.repeats[same];
    else if (walk.lines.size() < kMaxLines)
    {
        walk.lines.append(line);
        walk.repeats.append(1);
    }
    else
        ++walk.hidden;
}

bool isText(const QQuickItem* item) { return item->inherits("QQuickText"); }

bool isLayout(const QQuickItem* item) { return item->inherits("QQuickLayout"); }

bool isFlickable(const QQuickItem* item) { return item->inherits("QQuickFlickable"); }

bool isEmptyText(const QQuickItem* item)
{
    return isText(item) && item->property("text").toString().isEmpty();
}

bool isShown(const QQuickItem* item) { return item->isVisible() && item->opacity() > 0.0; }

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

bool carriesText(const QQuickItem* item)
{
    if (!isShown(item))
        return false;
    if (isText(item))
        return !isEmptyText(item);
    const QList<QQuickItem*> children = item->childItems();
    for (const QQuickItem* child : children)
    {
        if (carriesText(child))
            return true;
    }
    return false;
}

QRectF rectIn(QQuickItem* item, QQuickItem* space)
{
    return item->mapRectToItem(space, QRectF(0, 0, item->width(), item->height()));
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

bool scrolls(QQuickItem* item, Qt::Orientation orientation)
{
    if (!isFlickable(item) || !item->property("interactive").toBool())
        return false;

    const bool horizontal = orientation == Qt::Horizontal;
    if (item->property("flickableDirection").toInt()
        == (horizontal ? kVerticalFlick : kHorizontalFlick))
        return false;

    const qreal content = item->property(horizontal ? "contentWidth" : "contentHeight").toReal();
    return content > (horizontal ? item->width() : item->height()) + kSlack;
}

Clip clipOf(QQuickItem* item, const Walk& walk)
{
    Clip clip;
    clip.item = item;
    clip.rect = rectIn(item, walk.root);
    clip.scrollsX = scrolls(item, Qt::Horizontal);
    clip.scrollsY = scrolls(item, Qt::Vertical);
    return clip;
}

void checkLayout(QQuickItem* item, Walk& walk, Branch& branch)
{
    if (item->width() < kSlack || item->height() < kSlack)
        return;

    if (!branch.heightDone && tooShort(item) && !aChildLayoutIs(item, tooShort))
    {
        branch.heightDone = true;
        add(walk,
            QStringLiteral("%1 needs %2px of height but has %3px: the bottom is cut off (set "
                           "scrollable: true on the page, or let one child shrink with "
                           "Layout.fillHeight: true)")
                .arg(describe(item, walk))
                .arg(qRound(childrenBottom(item)))
                .arg(qRound(item->height())));
    }
    if (!branch.widthDone && tooNarrow(item) && !aChildLayoutIs(item, tooNarrow))
    {
        branch.widthDone = true;
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

bool checkEdges(QQuickItem* item, Walk& walk, Branch& branch)
{
    const QRectF rect = rectIn(item, walk.root);
    const QString name = label(item);
    bool reported = false;

    if (!branch.widthDone && rect.right() > walk.root->width() + kSlack)
    {
        branch.widthDone = true;
        reported = true;
        add(walk,
            QStringLiteral("%1 sticks out of the screen on the right by %2px")
                .arg(name)
                .arg(qRound(rect.right() - walk.root->width())));
    }
    if (!branch.heightDone && !branch.inScroll && rect.bottom() > walk.root->height() + kSlack
        && rect.top() < walk.root->height())
    {
        reported = true;
        add(walk,
            QStringLiteral("%1 runs past the bottom of the screen by %2px")
                .arg(name)
                .arg(qRound(rect.bottom() - walk.root->height())));
    }
    return reported;
}

void checkSize(QQuickItem* item, Walk& walk)
{
    const QVariant count = item->property("count");
    if (count.isValid() && count.toInt() == 0)
        return;
    if (!showsSomething(item) || isEmptyText(item))
        return;

    if (item->height() < kSlack)
        add(walk, QStringLiteral("%1 has no height, so it is invisible").arg(label(item)));
    else if (item->width() < kSlack)
        add(walk, QStringLiteral("%1 has no width, so it is invisible").arg(label(item)));
}

bool cutByItsLineLimit(const QQuickItem* item)
{
    const int limit = item->property("maximumLineCount").toInt();
    return item->property("wrapMode").toInt() != kNoWrap && limit >= kMinWrappedLines
        && item->property("lineCount").toInt() >= limit;
}

void checkText(QQuickItem* item, Walk& walk)
{
    if (isEmptyText(item) || item->width() < kSlack || item->height() < kSlack)
        return;
    const bool tooWide = item->property("contentWidth").toReal() > item->width() + kSlack;
    if (!tooWide && (!item->property("truncated").toBool() || cutByItsLineLimit(item)))
        return;

    QString line = QStringLiteral("%1: the text \"%2\" does not fit")
                       .arg(describe(item, walk))
                       .arg(shorten(item->property("text").toString()));
    if (item->implicitWidth() > item->width() + kSlack)
        line += QStringLiteral(": it needs %1px of width but has %2px")
                    .arg(qRound(item->implicitWidth()))
                    .arg(qRound(item->width()));
    add(walk,
        line
            + QStringLiteral(" (give it more width, shorten the text, or for text from the "
                             "user's data let it wrap)"));
}

bool checkClip(QQuickItem* item, Walk& walk, const Branch& branch)
{
    const Clip& clip = branch.clip;
    if (!clip.item || clip.reported || item == clip.item || !showsSomething(item)
        || isEmptyText(item))
        return false;

    const QRectF rect = rectIn(item, walk.root);
    QStringList sides;
    if (!clip.scrollsX)
    {
        if (clip.rect.left() - rect.left() > kSlack)
            sides.append(
                QStringLiteral("on the left by %1px").arg(qRound(clip.rect.left() - rect.left())));
        if (!branch.widthDone && rect.right() - clip.rect.right() > kSlack)
            sides.append(QStringLiteral("on the right by %1px")
                             .arg(qRound(rect.right() - clip.rect.right())));
    }
    if (!clip.scrollsY)
    {
        if (clip.rect.top() - rect.top() > kSlack)
            sides.append(
                QStringLiteral("at the top by %1px").arg(qRound(clip.rect.top() - rect.top())));
        if (!branch.heightDone && rect.bottom() - clip.rect.bottom() > kSlack)
            sides.append(QStringLiteral("at the bottom by %1px")
                             .arg(qRound(rect.bottom() - clip.rect.bottom())));
    }
    if (sides.isEmpty())
        return false;

    add(walk,
        QStringLiteral("%1 is cut off by %2 %3 (make it smaller or give its container more room)")
            .arg(describe(item, walk))
            .arg(describe(clip.item, walk))
            .arg(sides.join(QStringLiteral(", "))));
    return true;
}

QList<Sibling> siblingsOf(QQuickItem* parent, const Walk& walk)
{
    QList<Sibling> siblings;
    const QList<QQuickItem*> children = parent->childItems();
    for (QQuickItem* child : children)
    {
        if (!isShown(child) || child->inherits("QQuickRepeater") || !ownedBy(child, walk.dir))
            continue;
        if (child->width() < kSlack || child->height() < kSlack || !showsSomething(child)
            || isEmptyText(child))
            continue;
        siblings.append({ child, rectIn(child, parent) });
    }
    return siblings;
}

bool covers(const QRectF& outer, const QRectF& inner)
{
    return outer.adjusted(-kTolerance, -kTolerance, kTolerance, kTolerance).contains(inner);
}

Overlap overlapOf(const Sibling& first, const Sibling& second)
{
    const QRectF common = first.rect.intersected(second.rect);
    if (common.width() <= kTolerance || common.height() <= kTolerance)
        return Overlap();

    const bool oneCoversTheOther
        = covers(first.rect, second.rect) || covers(second.rect, first.rect);
    if (oneCoversTheOther && !(carriesText(first.item) && carriesText(second.item)))
        return Overlap();

    Overlap overlap;
    overlap.vertical = common.height() < common.width();
    overlap.depth = overlap.vertical ? common.height() : common.width();
    return overlap;
}

QString overlapLine(const Sibling& first, const Sibling& second, const Overlap& overlap,
                    const Walk& walk)
{
    const QString size
        = QStringLiteral("%1px of %2")
              .arg(qRound(overlap.depth))
              .arg(overlap.vertical ? QStringLiteral("height") : QStringLiteral("width"));
    const QString advice = QStringLiteral("they are drawn on top of each other: give each its "
                                          "own space, for example inside a Column or Row");
    if (ownerOf(first.item, walk) == ownerOf(second.item, walk))
        return QStringLiteral("%1: %2 overlaps %3 by %4 (%5)")
            .arg(describe(first.item, walk), named(first.item), named(second.item), size, advice);
    return QStringLiteral("%1 overlaps %2 by %3 (%4)")
        .arg(describe(first.item, walk), describe(second.item, walk), size, advice);
}

void checkOverlap(QQuickItem* parent, Walk& walk)
{
    const QList<Sibling> siblings = siblingsOf(parent, walk);
    QSet<QQuickItem*> involved;
    QString first;

    for (int i = 0; i < siblings.size(); ++i)
    {
        for (int j = i + 1; j < siblings.size(); ++j)
        {
            const Overlap overlap = overlapOf(siblings[i], siblings[j]);
            if (overlap.depth <= 0)
                continue;
            if (involved.isEmpty())
                first = overlapLine(siblings[i], siblings[j], overlap, walk);
            involved.insert(siblings[i].item);
            involved.insert(siblings[j].item);
        }
    }
    if (involved.isEmpty())
        return;

    if (involved.size() > 2)
        first += QStringLiteral("; %1 more overlap in the same place").arg(involved.size() - 2);
    add(walk, first);
}

void walkItem(QQuickItem* item, Walk& walk, Branch branch)
{
    if (!isShown(item))
        return;

    item->ensurePolished();

    if (ownedBy(item, walk.dir))
        walk.files.insert(fileOf(item));

    if (isLayout(item))
        checkLayout(item, walk, branch);

    if (item != walk.root && !item->inherits("QQuickRepeater"))
    {
        bool cut = false;
        if (ownedBy(item, walk.dir))
        {
            cut = checkEdges(item, walk, branch) || checkClip(item, walk, branch);
            checkSize(item, walk);
        }
        if (isText(item))
            checkText(item, walk);
        if (cut)
            branch.clip.reported = true;
    }

    if (isFlickable(item) && item->property("interactive").toBool())
        branch.inScroll = true;
    if (item->clip())
        branch.clip = clipOf(item, walk);

    checkOverlap(item, walk);

    const QList<QQuickItem*> children = item->childItems();
    for (QQuickItem* child : children)
        walkItem(child, walk, branch);
}
}

QString AppLayout::Report::text() const
{
    return QStringLiteral("Measured with %1 on screen\n%2")
        .arg(files, fits() ? QStringLiteral("The layout fits the screen") : problems);
}

AppLayout::Report AppLayout::measure(QQuickItem* root, const QString& appDir)
{
    if (!root)
        return Report();

    Walk walk { root, appDir, QStringList(), 0, QList<int>(), QSet<QString>() };
    walkItem(root, walk, Branch());

    QStringList files = walk.files.values();
    files.sort();
    Report report;
    report.files = files.join(QStringLiteral(", "));
    if (walk.lines.isEmpty())
        return report;

    for (int i = 0; i < walk.lines.size(); ++i)
        if (walk.repeats[i] > 1)
            walk.lines[i] += QStringLiteral(" (%1 times)").arg(walk.repeats[i]);

    report.problems = QStringLiteral("Layout problems on a %1x%2 screen:\n- %3")
                          .arg(qRound(root->width()))
                          .arg(qRound(root->height()))
                          .arg(walk.lines.join(QStringLiteral("\n- ")));
    if (walk.hidden > 0)
        report.problems += QStringLiteral("\n- and %1 more").arg(walk.hidden);
    return report;
}

AppLayout::Report AppLayout::measureApp(const QString& appId)
{
    AppLoader* loader = AppLoader::running(appId);
    if (!loader || !loader->item())
        return Report();
    return measure(loader->item(), App(appId).dir());
}
