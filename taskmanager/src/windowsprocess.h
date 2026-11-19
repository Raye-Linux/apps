// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Windows programs in the process list. The layer that runs them starts
// every program through its loader, so /proc/<pid>/exe names the loader
// and the command line holds the Windows path of the program. The layer
// also runs helpers of its own (its server, services.exe, winedevice.exe,
// explorer.exe...): those are shown as one thing, never by their names.

#pragma once

#include <QByteArray>
#include <QString>

namespace Raye::TaskManager
{

enum class WindowsRole { None, Program, Support };

struct WindowsProcess {
    WindowsRole role = WindowsRole::None;
    QString file; // "notepad++.exe", for a program
};

// exe is the target of /proc/<pid>/exe, cmdline the raw /proc/<pid>/cmdline
WindowsProcess windowsProcess(const QString &exe, const QByteArray &cmdline);

// the app the layer's helpers are grouped under
inline constexpr char16_t WindowsSupportAppId[] = u"org.raye.windows.support";

// "windows:notepad++.exe", the app of a program its scope says nothing about;
// the case is kept, it is the name shown when no Start entry knows the file
QString windowsProgramAppId(const QString &file);
// the file back from such an id; empty for any other id
QString windowsProgramFile(const QString &appId);
// "notepad++" for "notepad++.exe": the name when no Start entry knows it
QString windowsProgramName(const QString &file);

// a scope made by the opener of any Windows program (a double click on an
// .exe), not by the program's own Start entry
bool isGenericWindowsLauncher(const QString &appId);

} // namespace Raye::TaskManager
