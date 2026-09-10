#include "runtime/app/appfiles.h"

#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>

namespace
{
constexpr int kMaxReadLines = 2000;
constexpr int kMaxMatches = 50;
constexpr int kMaxLineLength = 200;
constexpr int kMaxRegions = 3;

const QRegularExpression& namePattern()
{
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z][A-Za-z0-9_]*\\.(qml|js)$"));
    return pattern;
}

bool isSourceName(const QString& name) { return namePattern().match(name).hasMatch(); }

const QRegularExpression& functionPattern()
{
    static const QRegularExpression pattern(
        QStringLiteral("^\\s*function\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\(([^)]*)\\)"));
    return pattern;
}

QStringList functionsIn(const QStringList& lines)
{
    QStringList functions;
    for (const QString& line : lines)
    {
        const QRegularExpressionMatch match = functionPattern().match(line);
        if (match.hasMatch())
            functions.append(match.captured(1) + QLatin1Char('(') + match.captured(2).simplified()
                             + QLatin1Char(')'));
    }
    return functions;
}

QString elide(const QString& text)
{
    if (text.size() <= kMaxLineLength)
        return text;
    return text.left(kMaxLineLength) + QStringLiteral("…");
}

QString numbered(int line, QChar marker, const QString& text)
{
    const QString prefix = QStringLiteral("%1%2").arg(line, 4).arg(marker);
    if (text.isEmpty())
        return prefix;
    return prefix + QLatin1Char(' ') + elide(text);
}

QStringList splitLines(const QString& content)
{
    QStringList lines = content.split(QLatin1Char('\n'));
    if (!lines.isEmpty() && lines.constLast().isEmpty())
        lines.removeLast();
    for (QString& line : lines)
    {
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
    }
    return lines;
}

int lineOf(const QString& content, qsizetype offset)
{
    return static_cast<int>(content.left(offset).count(QLatin1Char('\n'))) + 1;
}

struct Span
{
    qsizetype offset;
    qsizetype length;
};

QList<Span> exactMatches(const QString& content, const QString& needle)
{
    QList<Span> spans;
    for (qsizetype at = content.indexOf(needle); at >= 0;
         at = content.indexOf(needle, at + needle.size()))
        spans.append(Span { at, needle.size() });
    return spans;
}

QString withoutTrailingBlanks(const QString& text, QList<qsizetype>* origin)
{
    QString out;
    QList<qsizetype> pending;
    for (qsizetype i = 0; i < text.size(); ++i)
    {
        const QChar c = text.at(i);
        if (c == QLatin1Char(' ') || c == QLatin1Char('\t'))
        {
            pending.append(i);
            continue;
        }
        if (c != QLatin1Char('\n'))
        {
            for (qsizetype at : std::as_const(pending))
            {
                out.append(text.at(at));
                if (origin)
                    origin->append(at);
            }
        }
        pending.clear();
        out.append(c);
        if (origin)
            origin->append(i);
    }
    return out;
}

QList<Span> looseMatches(const QString& content, const QString& needle)
{
    QList<qsizetype> origin;
    const QString loose = withoutTrailingBlanks(content, &origin);
    QString target = withoutTrailingBlanks(needle, nullptr);
    while (!target.isEmpty() && target.back().isSpace())
        target.chop(1);
    if (target.isEmpty())
        return {};

    QList<Span> spans;
    const QList<Span> found = exactMatches(loose, target);
    for (const Span& span : found)
    {
        const qsizetype start = origin.at(span.offset);
        const qsizetype end = origin.at(span.offset + span.length - 1) + 1;
        spans.append(Span { start, end - start });
    }
    return spans;
}

bool loadFile(const QString& path, QString* content, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        *error = QStringLiteral("Cannot read %1: %2")
                     .arg(QFileInfo(path).fileName(), file.errorString());
        return false;
    }
    *content = QString::fromUtf8(file.readAll());
    return true;
}

bool saveFile(const QString& path, const QString& content, QString* error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        *error = QStringLiteral("Cannot write %1: %2")
                     .arg(QFileInfo(path).fileName(), file.errorString());
        return false;
    }

    file.write(content.toUtf8());
    if (!file.commit())
    {
        *error = QStringLiteral("Cannot write %1: %2")
                     .arg(QFileInfo(path).fileName(), file.errorString());
        return false;
    }
    return true;
}
}

AppFiles::AppFiles(QString dir)
    : m_dir(std::move(dir))
{
}

Result<QString> AppFiles::list() const
{
    if (m_dir.isEmpty())
        return Result<QString>::failure(QStringLiteral("No app directory"));

    const QStringList names = sourceNames();
    if (names.isEmpty())
        return Result<QString>::failure(QStringLiteral("No files"));

    QStringList lines;
    for (const QString& name : names)
    {
        QString content;
        QString error;
        if (!loadFile(m_dir + QLatin1Char('/') + name, &content, &error))
            continue;
        lines.append(QStringLiteral("%1 %2L").arg(name).arg(splitLines(content).size()));
    }

    return Result<QString>::success(lines.join(QLatin1Char('\n')));
}

Result<QString> AppFiles::outline() const
{
    if (m_dir.isEmpty())
        return Result<QString>::failure(QStringLiteral("No app directory"));

    const QStringList names = sourceNames();
    if (names.isEmpty())
        return Result<QString>::failure(QStringLiteral("No files"));

    QStringList out;
    for (const QString& name : names)
    {
        QString content;
        QString error;
        if (!loadFile(m_dir + QLatin1Char('/') + name, &content, &error))
            continue;

        const QStringList lines = splitLines(content);
        out.append(QStringLiteral("%1 %2L").arg(name).arg(lines.size()));
        const QStringList functions = functionsIn(lines);
        for (const QString& function : functions)
            out.append(QStringLiteral("  ") + function);
    }

    return Result<QString>::success(out.join(QLatin1Char('\n')));
}

Result<QString> AppFiles::read(const QString& path, int firstLine, int lineCount) const
{
    QString error;
    const QString absolute = resolve(path, &error);
    if (absolute.isEmpty())
        return Result<QString>::failure(error);

    if (!QFileInfo::exists(absolute))
        return Result<QString>::failure(missing(path));

    QString content;
    if (!loadFile(absolute, &content, &error))
        return Result<QString>::failure(error);

    const QStringList lines = splitLines(content);
    const int total = static_cast<int>(lines.size());
    const int from = qMax(1, firstLine);
    if (from > total)
        return Result<QString>::failure(QStringLiteral("%1 has %2 lines").arg(path).arg(total));

    const int count = qMin(lineCount > 0 ? lineCount : total - from + 1, kMaxReadLines);
    const int to = qMin(total, from + count - 1);

    QStringList out { QStringLiteral("%1 %2L").arg(path).arg(total) };
    for (int line = from; line <= to; ++line)
        out.append(numbered(line, QLatin1Char(':'), lines.at(line - 1)));

    return Result<QString>::success(out.join(QLatin1Char('\n')));
}

Result<QString> AppFiles::grep(const QString& text, int context, const QString& path) const
{
    if (text.isEmpty())
        return Result<QString>::failure(QStringLiteral("Empty pattern"));

    QStringList names;
    if (path.isEmpty())
    {
        names = sourceNames();
        if (names.isEmpty())
            return Result<QString>::failure(QStringLiteral("No files"));
    }
    else
    {
        QString error;
        if (resolve(path, &error).isEmpty())
            return Result<QString>::failure(error);
        if (!QFileInfo::exists(m_dir + QLatin1Char('/') + path))
            return Result<QString>::failure(missing(path));
        names.append(path);
    }

    const int radius = qMax(0, context);
    QStringList out;
    int found = 0;
    int shown = 0;

    for (const QString& name : names)
    {
        QString content;
        QString error;
        if (!loadFile(m_dir + QLatin1Char('/') + name, &content, &error))
            continue;

        const QStringList lines = splitLines(content);
        QList<int> hits;
        for (int i = 0; i < lines.size(); ++i)
        {
            if (lines.at(i).contains(text))
                hits.append(i);
        }

        found += static_cast<int>(hits.size());
        if (hits.isEmpty())
            continue;

        const QSet<int> marked(hits.constBegin(), hits.constEnd());
        QStringList block;
        int last = -1;
        for (int hit : hits)
        {
            if (shown >= kMaxMatches)
                break;
            ++shown;

            const int begin = qMax(0, hit - radius);
            const int end = qMin(static_cast<int>(lines.size()) - 1, hit + radius);
            if (last >= 0 && begin > last + 1)
                block.append(QStringLiteral("--"));

            for (int i = qMax(begin, last + 1); i <= end; ++i)
            {
                block.append(numbered(
                    i + 1, marked.contains(i) ? QLatin1Char('>') : QLatin1Char(' '), lines.at(i)));
            }
            last = qMax(last, end);
        }

        if (block.isEmpty())
            continue;

        if (!out.isEmpty())
            out.append(QString());
        out.append(name);
        out.append(block);
    }

    if (found == 0)
        return Result<QString>::success(QStringLiteral("No matches for \"%1\"").arg(text));

    if (shown < found)
    {
        out.append(
            QStringLiteral("%1 matches, showing %2 — narrow the pattern").arg(found).arg(shown));
    }

    return Result<QString>::success(out.join(QLatin1Char('\n')));
}

Result<QString> AppFiles::write(const QString& path, const QString& content)
{
    QString error;
    const QString absolute = resolve(path, &error);
    if (absolute.isEmpty())
        return Result<QString>::failure(error);

    if (content.trimmed().isEmpty())
        return Result<QString>::failure(QStringLiteral("%1 would be empty").arg(path));

    if (!QDir().mkpath(m_dir))
        return Result<QString>::failure(QStringLiteral("Cannot create %1").arg(m_dir));

    if (!saveFile(absolute, content, &error))
        return Result<QString>::failure(error);

    return Result<QString>::success(
        QStringLiteral("%1 %2L written").arg(path).arg(splitLines(content).size()));
}

Result<QString> AppFiles::replace(const QString& path, const QString& before, const QString& after,
                                  bool all)
{
    QString error;
    const QString absolute = resolve(path, &error);
    if (absolute.isEmpty())
        return Result<QString>::failure(error);

    if (before.isEmpty())
        return Result<QString>::failure(QStringLiteral("Empty anchor"));

    if (!QFileInfo::exists(absolute))
        return Result<QString>::failure(missing(path));

    QString content;
    if (!loadFile(absolute, &content, &error))
        return Result<QString>::failure(error);

    QList<Span> spans = exactMatches(content, before);
    if (spans.isEmpty())
        spans = looseMatches(content, before);

    if (spans.isEmpty())
        return Result<QString>::failure(QStringLiteral("Anchor not found in %1").arg(path));

    if (spans.size() > 1 && !all)
    {
        QStringList at;
        for (const Span& span : spans)
            at.append(QString::number(lineOf(content, span.offset)));
        return Result<QString>::failure(QStringLiteral("%1 matches at lines %2 — extend the anchor")
                                            .arg(spans.size())
                                            .arg(at.join(QStringLiteral(", "))));
    }

    QString updated;
    QList<qsizetype> changes;
    qsizetype from = 0;
    for (const Span& span : spans)
    {
        updated += content.mid(from, span.offset - from);
        changes.append(updated.size());
        updated += after;
        from = span.offset + span.length;
        if (!all)
            break;
    }
    updated += content.mid(from);

    if (!saveFile(absolute, updated, &error))
        return Result<QString>::failure(error);

    const QStringList lines = splitLines(updated);
    const int total = static_cast<int>(lines.size());
    const int count = static_cast<int>(changes.size());
    QStringList out { QStringLiteral("%1 %2 replacement%3")
                          .arg(path)
                          .arg(count)
                          .arg(count == 1 ? QString() : QStringLiteral("s")) };

    int newlines = static_cast<int>(after.count(QLatin1Char('\n')));
    if (newlines > 0 && after.endsWith(QLatin1Char('\n')))
        --newlines;

    const int regions = qMin(count, kMaxRegions);
    for (int i = 0; i < regions; ++i)
    {
        const int start = lineOf(updated, changes.at(i));
        const int end = start + newlines;
        if (i > 0)
            out.append(QStringLiteral("--"));

        for (int line = qMax(1, start - 1); line <= qMin(total, end + 1); ++line)
        {
            const bool changed = line >= start && line <= end;
            out.append(
                numbered(line, changed ? QLatin1Char('>') : QLatin1Char(' '), lines.at(line - 1)));
        }
    }

    if (count > regions)
        out.append(QStringLiteral("+%1 more").arg(count - regions));

    return Result<QString>::success(out.join(QLatin1Char('\n')));
}

Result<QString> AppFiles::remove(const QString& path)
{
    QString error;
    const QString absolute = resolve(path, &error);
    if (absolute.isEmpty())
        return Result<QString>::failure(error);

    if (path == QStringLiteral("Main.qml"))
        return Result<QString>::failure(QStringLiteral("Main.qml is the entry point"));

    if (!QFileInfo::exists(absolute))
        return Result<QString>::failure(missing(path));

    if (!QFile::remove(absolute))
        return Result<QString>::failure(QStringLiteral("Cannot remove %1").arg(path));

    return Result<QString>::success(QStringLiteral("%1 removed").arg(path));
}

QString AppFiles::resolve(const QString& path, QString* error) const
{
    if (m_dir.isEmpty())
    {
        *error = QStringLiteral("No app directory");
        return QString();
    }

    if (!isSourceName(path))
    {
        *error = QStringLiteral("Invalid file name \"%1\": use Name.qml or name.js").arg(path);
        return QString();
    }

    return m_dir + QLatin1Char('/') + path;
}

AppSources AppFiles::snapshot() const
{
    AppSources sources;
    const QStringList names = sourceNames();
    for (const QString& name : names)
    {
        QString content;
        QString error;
        if (loadFile(m_dir + QLatin1Char('/') + name, &content, &error))
            sources.insert(name, content);
    }
    return sources;
}

void AppFiles::restore(const AppSources& sources) const
{
    const QStringList names = sourceNames();
    for (const QString& name : names)
    {
        if (!sources.contains(name))
            QFile::remove(m_dir + QLatin1Char('/') + name);
    }

    for (auto it = sources.cbegin(); it != sources.cend(); ++it)
    {
        QString error;
        saveFile(m_dir + QLatin1Char('/') + it.key(), it.value(), &error);
    }
}

QStringList AppFiles::sourceNames() const
{
    if (m_dir.isEmpty())
        return QStringList();

    QStringList names;
    const QStringList entries = QDir(m_dir).entryList(QDir::Files, QDir::Name);
    for (const QString& name : entries)
    {
        if (isSourceName(name))
            names.append(name);
    }
    return names;
}

QString AppFiles::missing(const QString& path) const
{
    const QStringList names = sourceNames();
    if (names.isEmpty())
        return QStringLiteral("No %1, and this app has no files").arg(path);
    return QStringLiteral("No %1 — files: %2").arg(path, names.join(QLatin1Char(' ')));
}
