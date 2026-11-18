// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-graphics, the graphics card panel. --performance and --fan open
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
    static const QStringList pages{QStringLiteral("--overview"), QStringLiteral("--performance"), QStringLiteral("--fan")};
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
    QGuiApplication::setApplicationName(QStringLiteral("raye-graphics"));
    QGuiApplication::setOrganizationName(QStringLiteral("raye"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("raye.org"));
    QGuiApplication::setApplicationVersion(QStringLiteral(RAYE_APPS_VERSION));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral(TRANSLATION_DOMAIN));
    QGuiApplication::setApplicationDisplayName(i18n("Graphics Card"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.raye.graphics"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.raye.graphics")));

    KDBusService service(KDBusService::Unique | KDBusService::NoExitOnFailure);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("org.raye.graphics", "Main");
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
