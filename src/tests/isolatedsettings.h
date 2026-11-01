#pragma once

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryDir>

class IsolatedSettings
{
public:
    IsolatedSettings()
        : m_organization(QCoreApplication::organizationName())
        , m_application(QCoreApplication::applicationName())
    {
        QCoreApplication::setOrganizationName(organization());
        QCoreApplication::setApplicationName(application());
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_directory.path());
    }

    ~IsolatedSettings()
    {
        QCoreApplication::setOrganizationName(m_organization);
        QCoreApplication::setApplicationName(m_application);
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                           QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
    }

    IsolatedSettings(const IsolatedSettings&) = delete;
    IsolatedSettings& operator=(const IsolatedSettings&) = delete;

    static QString organization() { return QStringLiteral("qanvastest"); }
    static QString application() { return QStringLiteral("qanvastest"); }

private:
    QTemporaryDir m_directory;
    QString m_organization;
    QString m_application;
};
