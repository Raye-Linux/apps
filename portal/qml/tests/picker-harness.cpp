// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// Shows the share picker with fake screens and windows, to look at it
// (under Xvfb for the screenshots). Not installed.
//
//   picker-harness [--screens 2560x1440,1920x1080] [--windows 6] [--types 3]
//                  [--lang fr] [--multiple] [--cursor 2] [--persist]
//                  [--tab screen|window|area] [--select 0,2] [--keyboard]
//                  [--app Discord] [--unknown] [--out shot.png] [--delay 800]
//                  [--icons Papirus-Dark]
//
// --types: 1 screens, 2 windows, 4 virtual screen (added up).
// --select: cards of the shown tab to choose. --keyboard: focus the grid
// and move once with the arrow keys, so the focus ring shows.
// --out: saves the window to a PNG after --delay ms and quits; without
// it the window stays until it is closed.

#include "fakesources.h"
#include "pickerloader.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

Q_IMPORT_QML_PLUGIN(org_raye_portal_pickerPlugin)

using namespace Qt::StringLiterals;

static QList<QSize> parseScreens(const QString &text)
{
    QList<QSize> sizes;
    for (const QString &part : text.split(u',', Qt::SkipEmptyParts)) {
        const QStringList wh = part.split(u'x');
        if (wh.size() == 2 && wh[0].toInt() > 0 && wh[1].toInt() > 0) {
            sizes.append(QSize(wh[0].toInt(), wh[1].toInt()));
        }
    }
    return sizes;
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption screens(u"screens"_s, u"Screen sizes"_s, u"list"_s, u"2560x1440"_s);
    const QCommandLineOption windows(u"windows"_s, u"Number of windows"_s, u"n"_s, u"6"_s);
    const QCommandLineOption types(u"types"_s, u"Source types"_s, u"n"_s, u"3"_s);
    const QCommandLineOption lang(u"lang"_s, u"Language"_s, u"code"_s, u"en"_s);
    const QCommandLineOption multiple(u"multiple"_s, u"Several sources"_s);
    const QCommandLineOption cursor(u"cursor"_s, u"Cursor mode"_s, u"n"_s, u"2"_s);
    const QCommandLineOption persist(u"persist"_s, u"The app asks to remember"_s);
    const QCommandLineOption tab(u"tab"_s, u"Tab shown"_s, u"id"_s);
    const QCommandLineOption select(u"select"_s, u"Cards to choose"_s, u"list"_s);
    const QCommandLineOption keyboard(u"keyboard"_s, u"Show the keyboard focus"_s);
    const QCommandLineOption appName(u"app"_s, u"App name"_s, u"name"_s, u"Discord"_s);
    const QCommandLineOption unknown(u"unknown"_s, u"The app is not known"_s);
    const QCommandLineOption out(u"out"_s, u"PNG to write"_s, u"file"_s);
    const QCommandLineOption delay(u"delay"_s, u"Wait before the shot"_s, u"ms"_s, u"800"_s);
    const QCommandLineOption icons(u"icons"_s, u"Icon theme (no desktop session under Xvfb)"_s, u"name"_s);
    const QCommandLineOption mo(u"mo"_s, u"Compiled catalog"_s, u"file"_s, QStringLiteral(RAYE_PICKER_MO));
    parser.addOptions({screens, windows, types, lang, multiple, cursor, persist, tab, select, keyboard, appName, unknown, out, delay, icons, mo});
    parser.process(app);
    if (parser.isSet(icons)) {
        QIcon::setThemeName(parser.value(icons));
    }

    if (!PickerLoader::useCatalog(parser.value(lang), parser.value(mo))) {
        qWarning("no catalog for %s: shown in English", qPrintable(parser.value(lang)));
    }

    QQmlApplicationEngine engine;
    const QString rayeUi = PickerLoader::findRayeUi(engine);
    if (rayeUi.isEmpty()) {
        qCritical("org.raye.ui not found: install raye-style or set RAYE_UI_QML_DIR");
        return 2;
    }
    qInfo().noquote() << "org.raye.ui:" << rayeUi;

    const uint sourceTypes = parser.value(types).toUInt();
    FakeOutputs *outputs = nullptr;
    FakeWindows *windowList = nullptr;
    if (sourceTypes & (1 | 4)) {
        outputs = new FakeOutputs(parseScreens(parser.value(screens)), sourceTypes, &engine);
    }
    if (sourceTypes & 2) {
        windowList = new FakeWindows(parser.value(windows).toInt(), &engine);
    }
    const QString name = parser.isSet(unknown) ? (parser.value(lang) == u"fr"_s ? u"Une application"_s : u"An app"_s) : parser.value(appName);

    QString error;
    QQuickWindow *window = PickerLoader::load(engine,
                                              {
                                                  {u"title"_s, u"Share screen with %1"_s.arg(name)},
                                                  {u"appId"_s, u"com.discordapp.Discord"_s},
                                                  {u"appName"_s, name},
                                                  {u"appIconName"_s, parser.isSet(unknown) ? u"application-x-executable"_s : u"com.discordapp.Discord"_s},
                                                  {u"appKnown"_s, !parser.isSet(unknown)},
                                                  {u"outputsModel"_s, QVariant::fromValue<QObject *>(outputs)},
                                                  {u"windowsModel"_s, QVariant::fromValue<QObject *>(windowList)},
                                                  {u"multiple"_s, parser.isSet(multiple)},
                                                  {u"sourceTypes"_s, sourceTypes},
                                                  {u"cursorMode"_s, parser.value(cursor).toUInt()},
                                                  {u"persistRequested"_s, parser.isSet(persist)},
                                                  {u"previewComponent"_s, QVariant::fromValue(PickerLoader::fakePreview(engine))},
                                              },
                                              &error);
    if (!window) {
        qCritical().noquote() << "the picker did not load:" << error;
        return 1;
    }
    if (outputs) {
        QObject::connect(window, SIGNAL(clearSelection()), outputs, SLOT(clearSelection()));
    }
    if (windowList) {
        QObject::connect(window, SIGNAL(clearSelection()), windowList, SLOT(clearSelection()));
    }
    QObject::connect(window, SIGNAL(accept()), &app, SLOT(quit()));
    QObject::connect(window, SIGNAL(reject()), &app, SLOT(quit()));

    if (parser.isSet(tab)) {
        window->setProperty("currentTab", parser.value(tab));
    }
    for (const QString &card : parser.value(select).split(u',', Qt::SkipEmptyParts)) {
        QVariant result;
        QMetaObject::invokeMethod(window, "choose", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, card.toInt()));
    }

    const QRect area = window->screen()->geometry();
    window->setPosition(area.center() - QPoint(window->width() / 2, window->height() / 2));
    QMetaObject::invokeMethod(window, "present");

    if (parser.isSet(keyboard)) {
        QTimer::singleShot(400, window, [window] {
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
            QKeyEvent release(QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &press);
            QCoreApplication::sendEvent(window, &release);
        });
    }
    if (parser.isSet(out)) {
        const QString file = parser.value(out);
        QTimer::singleShot(parser.value(delay).toInt(), window, [window, file] {
            const bool saved = window->grabWindow().save(file);
            qInfo().noquote() << (saved ? "saved" : "could not save") << file;
            QCoreApplication::exit(saved ? 0 : 3);
        });
    }
    return app.exec();
}
