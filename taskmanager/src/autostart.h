// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Startup apps page: the XDG autostart entries, and turning one off.
//
// The entries live in /etc/xdg/autostart (the system's) and in
// ~/.config/autostart (the person's). A file of the same name in the
// person's folder replaces the system one, so turning a system entry off
// is a copy of it under the home with Hidden=true -- the system file is
// never touched, and turning it back on removes the key again.
//
// "Startup impact" has no equivalent on Linux. What is measured here
// is the processor time the entry's unit has used since sign-in (the
// systemd generator starts each entry as app-<id>@autostart.service), so
// an entry that is not running has no figure, and says so.

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace Raye::TaskManager
{

struct AutostartEntry {
    QString fileName; // "org.kde.kalendarac.desktop"
    QString name;
    QString icon;
    QString comment;
    QString exec;
    bool enabled = true;
    bool systemEntry = false; // a file of that name is in a system folder
    QString path; // the file that decides: the person's copy if there is one
};

// systemDirs in order of precedence, the first one winning. desktop is
// the session's name as XDG_CURRENT_DESKTOP gives it ("KDE").
QList<AutostartEntry> listAutostart(const QStringList &systemDirs, const QString &userDir, const QString &lang,
                                    const QString &desktop);

// Turns an entry on or off by writing the person's copy. Returns false
// when the file could not be written.
bool setAutostartEnabled(const AutostartEntry &entry, bool enabled, const QString &userDir);

// "app-org.kde.kalendarac@autostart.service" for "org.kde.kalendarac.desktop".
QString autostartUnitName(const QString &fileName);

// systemd's escaping of a name: everything but letters, digits, ':', '_'
// and '.' becomes \xNN, and so does a leading '.'.
QString escapeUnitName(const QString &text);

enum class Impact { NotMeasured, None, Low, Medium, High };

// From the microseconds of processor the unit used; negative when there is
// no unit to read.
Impact impactForCpuUsec(qlonglong usec);

// usage_usec of a cpu.stat file; -1 when absent.
qlonglong parseCpuStatUsage(const QByteArray &cpuStat);

} // namespace Raye::TaskManager
