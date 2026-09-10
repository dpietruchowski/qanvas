#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

#include "async/result.h"

using AppSources = QMap<QString, QString>;

class AppFiles
{
public:
    explicit AppFiles(QString dir);

    AppSources snapshot() const;
    void restore(const AppSources& sources) const;
    QStringList unused() const;
    QStringList undefinedCalls() const;

    Result<QString> list() const;
    Result<QString> outline() const;
    Result<QString> read(const QString& path, int firstLine = 1, int lineCount = 0) const;
    Result<QString> grep(const QString& text, int context = 2,
                         const QString& path = QString()) const;

    Result<QString> write(const QString& path, const QString& content);
    Result<QString> replace(const QString& path, const QString& before, const QString& after,
                            bool all = false);
    Result<QString> remove(const QString& path);

private:
    QString resolve(const QString& path, QString* error) const;
    QStringList sourceNames() const;
    QString missing(const QString& path) const;

    QString m_dir;
};
