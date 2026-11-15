// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// Which window asks what to share: the Raye picker when it is built in,
// the desktop's own chooser otherwise, or when the user asked for it in
// ~/.config/xdg-desktop-portal-rayerc:
//
//   [Picker]
//   Style=kde
//
// RAYE_PORTAL_PICKER=kde in the environment does the same, for tests.

#pragma once

#include <QByteArray>
#include <QString>
#include <QVariant>

namespace Raye
{

struct PickerChoice {
    QString module; // QML module the dialog is loaded from
    QString component; // its root component, a Window
    QString domain; // translation domain of its words
    bool raye = false;
};

PickerChoice choosePicker(bool rayePickerBuilt, const QString &configuredStyle, const QByteArray &environment);

// The choice for this build, config and environment.
PickerChoice currentPicker();

// The cursor mode to use: the one the app asked for, unless the Raye
// picker offered "Share the pointer" and the user turned it off.
// showPointer is the dialog's "showPointer" property (invalid when the
// dialog has none). Modes as in the ScreenCast portal: 1 hidden,
// 2 embedded, 4 metadata.
uint effectiveCursorMode(uint askedMode, const QVariant &showPointer);

} // namespace Raye
