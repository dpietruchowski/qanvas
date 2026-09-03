#pragma once

#include <QString>

class App;

class AppSchema
{
public:
    explicit AppSchema(const App& app);
    ~AppSchema();

    AppSchema(const AppSchema&) = delete;
    AppSchema& operator=(const AppSchema&) = delete;

    QString describe() const;

private:
    QString m_connectionName;
    bool m_open = false;
};
