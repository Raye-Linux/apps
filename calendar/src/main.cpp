// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-calendar. Started from Start, from the clock's calendar on the
// taskbar ("raye-calendar:?new=2026-11-11" opens the editor on that day,
// "?day=" opens the day) and from a reminder. A .ics file given here is
// imported.

#include "raye-apps-version.h"

#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>

#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QUrlQuery>
#include <QWindow>

namespace
{
// hands what the command line asks to the window: day, new event, or a file
void deliver(QQmlApplicationEngine &engine, const QStringList &args, const QString &cwd)
{
    QObject *root = engine.rootObjects().value(0);
    if (!root)
        return;
    for (const QString &arg : args.mid(1)) {
        const QUrl url = QUrl::fromUserInput(arg, cwd, QUrl::AssumeLocalFile);
        if (url.scheme() == QLatin1String("raye-calendar")) {
            const QUrlQuery q(url);
            const bool isNew = q.hasQueryItem(QStringLiteral("new"));
            const QString day = q.queryItemValue(isNew ? QStringLiteral("new") : QStringLiteral("day"));
            QMetaObject::invokeMethod(root, "showDay", Q_ARG(QVariant, day), Q_ARG(QVariant, isNew));
        } else if (url.isLocalFile()) {
            QMetaObject::invokeMethod(root, "importFile", Q_ARG(QVariant, url));
        }
    }
}
} // namespace

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("raye-calendar"));
    QGuiApplication::setOrganizationName(QStringLiteral("raye"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("raye.org"));
    QGuiApplication::setApplicationVersion(QStringLiteral(RAYE_APPS_VERSION));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral(TRANSLATION_DOMAIN));
    QGuiApplication::setApplicationDisplayName(i18n("Calendar"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.raye.calendar"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.raye.calendar")));

    // one window: a second start hands its arguments over here and exits
    KDBusService service(KDBusService::Unique | KDBusService::NoExitOnFailure);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("org.raye.calendar", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    deliver(engine, app.arguments(), QDir::currentPath());

    QObject::connect(&service, &KDBusService::activateRequested, &engine,
                     [&engine](const QStringList &args, const QString &cwd) {
                         if (auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst())) {
                             window->show();
                             window->raise();
                             window->requestActivate();
                         }
                         deliver(engine, args, cwd);
                     });

    return QGuiApplication::exec();
}
