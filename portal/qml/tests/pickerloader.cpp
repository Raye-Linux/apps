// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "pickerloader.h"

#include <KLocalizedContext>
#include <KLocalizedString>

#include <QDir>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTemporaryDir>

#include <clocale>

using namespace Qt::StringLiterals;

namespace PickerLoader
{

static bool canImport(const QStringList &extraPaths)
{
    QQmlEngine probe;
    for (const QString &path : extraPaths) {
        probe.addImportPath(path);
    }
    QQmlComponent component(&probe);
    component.setData("import QtQuick\nimport org.raye.ui\nimport org.raye.ui.effects\nQtObject {}\n", QUrl(u"probe.qml"_s));
    return component.status() == QQmlComponent::Ready;
}

QString findRayeUi(QQmlApplicationEngine &engine)
{
    if (canImport({})) {
        return u"installed"_s;
    }
    const QString fromEnv = qEnvironmentVariable("RAYE_UI_QML_DIR");
    if (!fromEnv.isEmpty() && canImport({fromEnv})) {
        engine.addImportPath(fromEnv);
        return u"env"_s;
    }
    const QStringList themeSource = {QStringLiteral(RAYE_THEME_QML_DIR), QStringLiteral(RAYE_UI_STUB_DIR)};
    if (QFile::exists(QStringLiteral(RAYE_THEME_QML_DIR "/org/raye/ui/qmldir")) && canImport(themeSource)) {
        for (const QString &path : themeSource) {
            engine.addImportPath(path);
        }
        return u"theme source"_s;
    }
    return {};
}

// gettext reads no catalog at all under the C locale (C.UTF-8 too):
// take any real UTF-8 locale for messages, the catalog picks the language
static bool realMessagesLocale()
{
    const QByteArray current = setlocale(LC_MESSAGES, nullptr);
    if (current != "C" && current != "POSIX" && !current.startsWith("C.")) {
        return true;
    }
    for (const char *name : {"fr_FR.UTF-8", "en_US.UTF-8", "en_GB.UTF-8", "fr_CA.UTF-8", "de_DE.UTF-8"}) {
        if (setlocale(LC_MESSAGES, name)) {
            return true;
        }
    }
    return false;
}

bool useCatalog(const QString &language, const QString &moFile)
{
    if (language.isEmpty() || language == u"en"_s) {
        KLocalizedString::setLanguages({u"en_US"_s});
        return true;
    }
    if (!QFile::exists(moFile) || !realMessagesLocale()) {
        return false;
    }
    // removed when the program ends
    static QTemporaryDir locales;
    if (!locales.isValid()) {
        return false;
    }
    const QString root = locales.path();
    const QString dir = root + u'/' + language + u"/LC_MESSAGES"_s;
    QDir().mkpath(dir);
    const QString target = dir + u"/xdg-desktop-portal-raye.mo"_s;
    QFile::remove(target);
    QFile::copy(moFile, target);
    KLocalizedString::addDomainLocaleDir("xdg-desktop-portal-raye", root);
    KLocalizedString::setLanguages({language});
    return true;
}

QQuickWindow *load(QQmlApplicationEngine &engine, const QVariantMap &properties, QString *error)
{
    auto context = new KLocalizedContext(&engine);
    context->setTranslationDomain(u"xdg-desktop-portal-raye"_s);
    engine.rootContext()->setContextObject(context);

    QStringList warnings;
    const auto collect = QObject::connect(&engine, &QQmlEngine::warnings, &engine, [&warnings](const QList<QQmlError> &list) {
        for (const QQmlError &warning : list) {
            warnings.append(warning.toString());
        }
    });
    engine.setInitialProperties(properties);
    engine.loadFromModule("org.raye.portal.picker", "SharePicker");
    QObject::disconnect(collect);
    const QList<QObject *> roots = engine.rootObjects();
    auto window = roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(roots.constLast());
    if (!window && error) {
        *error = warnings.join(u'\n');
    }
    return window;
}

QObject *fakePreview(QQmlApplicationEngine &engine)
{
    auto component = new QQmlComponent(&engine, QUrl::fromLocalFile(QStringLiteral(RAYE_PICKER_TESTS_DIR "/FakePreview.qml")), &engine);
    if (component->isError()) {
        qWarning() << component->errors();
        return nullptr;
    }
    return component;
}

} // namespace PickerLoader
