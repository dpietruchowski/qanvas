#include "runtime/chat/chatthread.h"

#include "runtime/storage/chatstore.h"

namespace
{
const QString kUser = QStringLiteral("user");
const QString kAssistant = QStringLiteral("assistant");
}

ChatThread::ChatThread(AgentController& controller, ChatStore& store, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_store(store)
{
    connect(&m_controller, &AgentController::busyChanged, this,
            [this]
            {
                if (!m_controller.isBusy())
                    rememberReply();
            });
}

void ChatThread::open(const QString& app)
{
    setApp(app);
    m_controller.restore(m_store.messages(app));
}

void ChatThread::bind(const QString& app)
{
    if (app == m_app)
        return;

    if (m_app.isEmpty())
        m_store.rebind(m_app, app);
    setApp(app);
}

void ChatThread::remember(const QString& text) { m_store.append(m_app, kUser, text); }

void ChatThread::forget(const QString& app)
{
    m_store.remove(app);
    if (app == m_app)
        setApp(QString());
}

void ChatThread::reset()
{
    m_store.remove(QString());
    setApp(QString());
    m_controller.restore({});
}

void ChatThread::setApp(const QString& app)
{
    if (app == m_app)
        return;

    m_app = app;
    emit appChanged();
}

void ChatThread::rememberReply()
{
    const QString reply = m_controller.lastResponse();
    if (!reply.isEmpty())
        m_store.append(m_app, kAssistant, reply);
}
