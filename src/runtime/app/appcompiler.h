#pragma once

#include "utils/result.h"

class App;
class QQmlEngine;

class AppCompiler
{
public:
    explicit AppCompiler(QQmlEngine& engine);

    Result<void> compile(const App& app) const;

private:
    QQmlEngine& m_engine;
};
