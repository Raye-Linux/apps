// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// Loads the share picker the way the backend does (src/quickdialog.cpp):
// QQmlApplicationEngine, the package's catalog through KLocalizedContext,
// initial properties, loadFromModule. Shared by the test and the harness.

#pragma once

#include <QString>
#include <QVariantMap>

class QQmlApplicationEngine;
class QQuickWindow;

namespace PickerLoader
{
// Makes org.raye.ui importable: as installed, from RAYE_UI_QML_DIR, or
// from the theme's source tree with a stand-in for its C++ part.
// Returns where it came from ("installed", "env", "theme source"), or
// an empty string when it is nowhere.
QString findRayeUi(QQmlApplicationEngine &engine);

// The French (or other) catalog compiled by this build, for languages
// other than English: copies <mo> into a locale tree and points KI18n at
// it. False when it cannot: no catalog, or no locale gettext would use.
bool useCatalog(const QString &language, const QString &moFile);

QQuickWindow *load(QQmlApplicationEngine &engine, const QVariantMap &properties, QString *error = nullptr);

// the drawn pictures of FakePreview.qml, as the picker's previewComponent
QObject *fakePreview(QQmlApplicationEngine &engine);
}
