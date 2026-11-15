// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// "Efficiency mode", and exactly what it is on Linux.
//
// Windows 11 puts a process in EcoQoS: lowest priority, and the small
// cores when the machine has them. Linux has no single switch for that.
// The nearest thing is the idle scheduling class, SCHED_IDLE, plus the
// idle I/O class. Measured on this desktop, two busy loops pinned to one
// processor for three seconds:
//
//   ordinary against ordinary   : 149 / 150 jiffies
//   ordinary against SCHED_IDLE : 298 /   1 jiffies
//   ordinary against nice 19    : 295 /   4 jiffies
//
// So a process in this mode gets, near enough, only the processor nobody
// else wants -- and when nothing else wants it, it runs at full speed.
// No weight can buy a SCHED_IDLE process a share back: the class is
// consulted before any weight is. The words the window shows must say
// that and not "lower priority", which sounds like a dial.
//
// And it is a ONE-WAY DOOR for its owner. A thread may always lower
// its own kind's priority and never raise it back: measured on this
// machine, an ordinary user switching a process of their own out of
// SCHED_IDLE gets EPERM, and so does renice from 19 back to 0. It only
// works with CAP_SYS_NICE, or when RLIMIT_NICE is 20 or more -- the
// boundary was measured one unit at a time: 19 refused, 20 allowed. The
// Debian default is 0. So the window must ask before it switches a
// process in, and must not draw a switch that cannot come back.

#pragma once

#include "proctable.h"

#include <QString>

namespace Raye::TaskManager
{

// Nice value asked for alongside the idle class. It changes little on
// its own (4 jiffies against 1), and it is what `ps` and `top` show, so
// the mode can be seen from outside this window.
constexpr int EfficiencyNice = 19;

// RLIMIT_NICE at or above this lets an unprivileged owner take a process
// back out of SCHED_IDLE. Measured, not read off a manual page.
constexpr int RlimitNiceToLeaveIdle = 20;

bool isEfficiencyMode(const ProcStat &stat);

// Whether the mode can be TURNED OFF again for a process of ours.
// rlimitNiceSoft is RLIMIT_NICE's soft limit as getrlimit() gives it.
bool canLeaveEfficiencyMode(int rlimitNiceSoft, bool hasCapSysNice);

// Whether setpriority() would accept target for a process of ours
// currently at current. Raising a priority needs the room above;
// asking for the value it already has always works.
bool canSetNice(int target, int current, int rlimitNiceSoft, bool hasCapSysNice);

// This process's own answers, read from the kernel, for the two above.
int rlimitNiceSoft();
bool hasCapSysNice();

enum class EfficiencyOutcome {
    Applied, // both halves done
    IoOnly, // the I/O class changed, the scheduling class did not
    Refused, // the kernel refused: not ours, or gone
    NotReversible, // asked to turn it off where the kernel will not allow it
};

// Puts pid in, or takes it out of, the mode. Turning it off is refused
// up front, with NotReversible, rather than half-done: the I/O class
// would come back and the scheduling class would not, leaving a process
// that looks ordinary in the table and still gets nothing.
EfficiencyOutcome setEfficiencyMode(int pid, bool on);

// The same, for a process read from a /proc tree of the tests' own: the
// identity is checked first, as for a signal.
EfficiencyOutcome setEfficiencyModeChecked(const ProcStat &asked, bool on, const QString &procRoot);

} // namespace Raye::TaskManager
