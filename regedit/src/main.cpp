// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-regedit, the Registry Editor. Started by org.raye.regedit.desktop,
// and by "regedit" in the Run box.

#include "raye-apps-version.h"

#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>

#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("raye-regedit"));
    QGuiApplication::setOrganizationName(QStringLiteral("raye"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("raye.org"));
    QGuiApplication::setApplicationVersion(QStringLiteral(RAYE_APPS_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.raye.regedit"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral(TRANSLATION_DOMAIN));
    QGuiApplication::setApplicationDisplayName(i18n("Registry Editor"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.raye.regedit"),
                                                    QIcon::fromTheme(QStringLiteral("preferences-other"))));

    KDBusService service(KDBusService::Unique | KDBusService::NoExitOnFailure);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("org.raye.regedit", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    // a .rayereg opened from the file manager: the window asks before importing
    const auto askImport = [&engine](const QStringList &args, const QString &cwd) {
        if (args.size() < 2)
            return;
        const QUrl file = QUrl::fromUserInput(args.at(1), cwd, QUrl::AssumeLocalFile);
        QMetaObject::invokeMethod(engine.rootObjects().constFirst(), "askImport", Q_ARG(QVariant, file));
    };
    askImport(app.arguments(), QDir::currentPath());
    QObject::connect(&service, &KDBusService::activateRequested, &engine,
                     [&engine, askImport](const QStringList &args, const QString &cwd) {
        if (auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst())) {
            window->show();
            window->raise();
            window->requestActivate();
        }
        askImport(args, cwd);
    });
    return QGuiApplication::exec();
}
