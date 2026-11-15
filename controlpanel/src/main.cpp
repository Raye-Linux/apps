// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-controlpanel, the Control Panel. Started by
// org.raye.controlpanel.desktop and by "control" in the Run box. Most of
// its items open a page of Raye Settings; System Tools is its own.

#include "raye-apps-version.h"

#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("raye-controlpanel"));
    QGuiApplication::setOrganizationName(QStringLiteral("raye"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("raye.org"));
    QGuiApplication::setApplicationVersion(QStringLiteral(RAYE_APPS_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.raye.controlpanel"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral(TRANSLATION_DOMAIN));
    QGuiApplication::setApplicationDisplayName(i18n("Control Panel"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.raye.controlpanel")));

    KDBusService service(KDBusService::Unique | KDBusService::NoExitOnFailure);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("org.raye.controlpanel", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    QObject::connect(&service, &KDBusService::activateRequested, &engine, [&engine]() {
        if (auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst())) {
            window->show();
            window->raise();
            window->requestActivate();
        }
    });
    return QGuiApplication::exec();
}
