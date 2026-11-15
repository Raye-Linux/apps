// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Grouping processes under the app they belong to, the way the Processes
// page shows them: "Apps" first, then "Background processes".
//
// The app of a process is read from its cgroup. The shell starts every
// app in a scope of its own, "app-<id>-<random>.scope" (or
// "app-<id>@<random>.service"), so the scope names the app and everything
// it starts -- a terminal and its shells, a browser and its tabs. Apps
// started at sign-in sit in "app-<id>@autostart.service": those are
// grouped too, but listed as background processes, which is where
// Windows puts what starts on its own.

#pragma once

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

namespace Raye::TaskManager
{

struct AppUnit {
    QString id; // the desktop file id, as the scope gives it
    bool autostart = false;

    bool isValid() const
    {
        return !id.isEmpty();
    }
};

// The app of one /proc/<pid>/cgroup file (cgroup v2: one line, "0::path").
AppUnit appUnitFromCgroup(const QByteArray &cgroupFile);

// The same for one unit name, "app-org.kde.konsole-3f2a9c.scope".
AppUnit appUnitFromName(const QString &unit);

// systemd's escaping: "\x2d" back to "-".
QString unescapeUnitName(const QString &text);

// The few keys of a .desktop file this app uses.
struct DesktopEntry {
    bool valid = false;
    QString name;
    QString icon;
    QString exec;
    QString comment;
    bool hidden = false;
    bool noDisplay = false;
    bool autostartEnabled = true; // X-GNOME-Autostart-enabled
    QStringList onlyShowIn;
    QStringList notShowIn;
};

// Reads the [Desktop Entry] group. lang is "fr" or "fr_CA"; Name[fr_CA]
// wins over Name[fr], which wins over Name.
DesktopEntry parseDesktopEntry(const QByteArray &text, const QString &lang);

// The candidates to look for, for an id taken from a scope: the id
// itself, then the id without the launcher's prefix ("flatpak-",
// "gnome-"...).
QStringList desktopIdCandidates(const QString &id);

// What the Processes page needs of one process.
struct ProcEntry {
    int pid = 0;
    int ppid = 0;
    QString name;
    char state = 'S';
    double cpu = 0.0;
    qlonglong memory = 0;
    double disk = -1.0; // bytes per second; -1 when it cannot be read
    bool efficiency = false;
    QString appId;
    QString appName;
    QString appIcon;
    bool autostart = false;
    bool kernelThread = false;
};

struct DisplayRow {
    enum Kind { Header, App, Child, Process };
    Kind kind = Process;
    int section = 0; // 0 apps, 1 background processes
    QString key; // "app:<id>" or "pid:<n>"; headers "section:<n>"
    QString name;
    QString icon;
    int pid = 0; // the lowest pid of a group
    int count = 0; // processes in the group; for a header, rows in the section
    double cpu = 0.0;
    qlonglong memory = 0;
    double disk = -1.0;
    bool efficiency = false;
    char state = 'S';
    bool expanded = false;
    QList<int> pids;
};

enum class SortKey { Name, Cpu, Memory, Disk, Pid };

// The rows of the Processes page: a header per section, an app row per
// group (and its processes under it when expanded), and the background
// processes that belong to no app; the kernel's own threads are left
// out, as Windows leaves out its kernel. filter keeps a row whose name or
// number contains the text; a group is kept with every process of it
// when the group's name matches, and with the matching ones otherwise.
QList<DisplayRow> buildDisplayRows(const QList<ProcEntry> &entries, const QSet<QString> &expanded, SortKey key,
                                   bool descending, const QString &filter);

// Every process started by pid, children first-level and down, in the
// order they were found. pid itself is not in the list.
QList<int> descendants(int pid, const QHash<int, int> &parentOf);

// Windows' priority classes, mapped onto nice values. Real-time is left
// out: SCHED_FIFO for a desktop program is how a machine stops answering.
enum class Priority { Low, BelowNormal, Normal, AboveNormal, High };

Priority priorityForNice(int nice);
int niceForPriority(Priority priority);

} // namespace Raye::TaskManager
