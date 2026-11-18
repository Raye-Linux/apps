// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-clock. Started from Start, from the clock on the taskbar and from
// a ring's notification: --alarms, --timers, --stopwatch and --world open
// that page.

#include "raye-apps-version.h"

#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

namespace
{
void deliver(QQmlApplicationEngine &engine, const QStringList &args)
{
    QObject *root = engine.rootObjects().value(0);
    if (!root)
        return;
    static const QStringList pages{QStringLiteral("--timers"), QStringLiteral("--alarms"), QStringLiteral("--stopwatch"),
                                   QStringLiteral("--world")};
    for (const QString &arg : args.mid(1)) {
        const int page = int(pages.indexOf(arg));
        if (page >= 0)
            QMetaObject::invokeMethod(root, "showPage", Q_ARG(QVariant, page));
    }
}
} // namespace

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("raye-clock"));
    QGuiApplication::setOrganizationName(QStringLiteral("raye"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("raye.org"));
    QGuiApplication::setApplicationVersion(QStringLiteral(RAYE_APPS_VERSION));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral(TRANSLATION_DOMAIN));
    QGuiApplication::setApplicationDisplayName(i18n("Clock"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.raye.clock"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.raye.clock")));

    // one window: a second start hands its arguments over here and exits
    KDBusService service(KDBusService::Unique | KDBusService::NoExitOnFailure);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("org.raye.clock", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    deliver(engine, app.arguments());

    QObject::connect(&service, &KDBusService::activateRequested, &engine, [&engine](const QStringList &args, const QString &) {
        if (auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst())) {
            window->show();
            window->raise();
            window->requestActivate();
        }
        deliver(engine, args);
    });

    return QGuiApplication::exec();
}
