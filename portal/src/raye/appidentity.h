// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// The name and icon shown for the app that asks to share the screen:
// "Discord wants to share your screen". Found from the portal's app id
// through the app's .desktop file; "An app" when it cannot be found.

#pragma once

#include <QString>
#include <QStringList>

namespace Raye
{

struct AppIdentity {
    QString id;
    QString name;
    QString iconName;
    bool known = false;
};

// appDirs: the folders searched for <app id>.desktop, first wins. Empty
// means the usual applications folders (XDG_DATA_HOME, XDG_DATA_DIRS).
AppIdentity appIdentity(const QString &appId, const QStringList &appDirs = {});

// Whether appId can be a file name: a reverse-DNS id or a plain name,
// never a path.
bool isPlausibleAppId(const QString &appId);

} // namespace Raye
