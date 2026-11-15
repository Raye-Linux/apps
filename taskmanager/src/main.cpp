// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-taskmanager. Started by its desktop file,
// org.raye.taskmanager.desktop -- the name the Raye shell asks for from
// the taskbar menu, the clock menu, the desktop menu, Run (taskmgr,
// resmon, perfmon) and the Ctrl+Alt+Delete screen.

#include "processmodel.h"
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
    QGuiApplication::setApplicationName(QStringLiteral("raye-taskmanager"));
    // the window's own choices go to ~/.config/raye/raye-taskmanager.conf
    QGuiApplication::setOrganizationName(QStringLiteral("raye"));
    QGuiApplication::setApplicationVersion(QStringLiteral(RAYE_APPS_VERSION));
    QGuiApplication::setApplicationDisplayName(i18n("Task Manager"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.raye.taskmanager"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("utilities-system-monitor")));

    KLocalizedString::setApplicationDomain(QByteArrayLiteral(TRANSLATION_DOMAIN));

    // one window: Ctrl+Shift+Esc a second time brings this one forward.
    // A second copy hands over here and exits, before loading any QML.
    QGuiApplication::setOrganizationDomain(QStringLiteral("raye.org"));
    KDBusService service(KDBusService::Unique | KDBusService::NoExitOnFailure);

    QQmlApplicationEngine engine;
    // the same line Raye Settings uses: it is what gives i18n() and
    // i18nc() to the QML of this package, in this package's catalogue
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("org.raye.taskmanager", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    QObject::connect(&service, &KDBusService::activateRequested, &engine, [&engine]() {
        if (auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst())) {
            window->show();
            window->raise();
            window->requestActivate();
        }
    });

    return QGuiApplication::exec();
}
