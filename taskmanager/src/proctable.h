// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Reading /proc for the Processes tab: one line of /proc/<pid>/stat, the
// machine's total jiffies, and the processor share between two samples.
//
// Nothing here says a word to the user: every number is raw, and the
// words that show it live in QML, in this package's catalogue. That is
// also what lets the tests link this against Qt::Core alone.
//
// The name in /proc/<pid>/stat is NOT a word. The kernel prints it
// between brackets and truncates it to 15 bytes, but it does not escape
// it: a program called "evil) 1 R 1" gives a line that splitting on
// spaces reads as a different process entirely, with a chosen state and
// a chosen parent. The only safe landmark is the LAST ')' of the line --
// a name cannot contain the bracket that closes it, because there is
// nothing after the closing bracket but numbers. parseProcStat() uses
// that landmark; see tests/test_proctable.cpp.

#pragma once

#include <QByteArray>
#include <QString>

namespace Raye::TaskManager
{

// SCHED_IDLE, the "efficiency mode" scheduling class. Spelled out rather
// than taken from <sched.h> so that a reader of the table sees what the
// number in field 41 of /proc/<pid>/stat means.
constexpr int SchedIdle = 5;
constexpr int SchedOther = 0;

// One line of /proc/<pid>/stat. Field numbers are the ones proc(5) uses
// (1-based), so a reader can check each one against the manual page.
struct ProcStat {
    bool valid = false;
    int pid = 0; // 1
    QString comm; // 2, brackets removed
    char state = '\0'; // 3: R S D Z T t W X x K W P I
    int ppid = 0; // 4
    qulonglong flags = 0; // 9, PF_* of the kernel
    qulonglong utime = 0; // 14, jiffies in user mode
    qulonglong stime = 0; // 15, jiffies in the kernel
    int nice = 0; // 19
    int threads = 0; // 20
    qulonglong startTime = 0; // 22, jiffies since boot
    qlonglong rssPages = 0; // 24, resident pages
    int policy = SchedOther; // 41

    qulonglong cpuJiffies() const
    {
        return utime + stime;
    }
    // a thread of the kernel's own (PF_KTHREAD): listed in Details, not
    // among the processes a person runs
    bool isKernelThread() const
    {
        return (flags & 0x00200000) != 0;
    }
};

// Parses one line of /proc/<pid>/stat. valid is false, and nothing else
// is filled in, when the line is not one.
ProcStat parseProcStat(const QByteArray &line);

// Total jiffies of the whole machine, from the first "cpu " line of
// /proc/stat. 0 when the text holds no such line.
//
// It is the "cpu " line with two spaces that is wanted, not "cpu0":
// a sum of the per-processor lines would count every tick as many times
// as there are processors.
qulonglong parseTotalJiffies(const QByteArray &procStat);

// What share of the machine a process used between two samples, in
// percent, Windows' way: 100 % is every processor at once, so a single
// thread spinning on an eight-processor machine shows 12.5 %.
//
// Two samples are needed, and the second must be later than the
// first. A single reading of utime+stime is the process's whole life,
// not what it is doing now: showing it would put a browser open since
// this morning at a steady 4 % that never moves. A call where the
// totals have not advanced returns 0, never a division by zero.
double cpuPercent(const ProcStat &before, const ProcStat &after, qulonglong totalBefore, qulonglong totalAfter);

// The name to show for a process. The kernel cuts comm at 15 bytes, so a
// name of that length is looked up in the command line: the program's
// file name when it starts with the cut name, comm otherwise.
QString displayName(const QString &comm, const QByteArray &cmdline);

// Resident memory in bytes, from field 24 and the page size.
//
// This is RSS: pages shared with another process are counted in
// both. The figure Windows calls "Memory" is nearer to Pss, which only
// /proc/<pid>/smaps_rollup gives, and which costs a walk of the whole
// address space per process. The Processes tab shows RSS and says so;
// smaps_rollup is for the Details tab, on the selected row alone.
qlonglong residentBytes(const ProcStat &stat, int pageSize);

} // namespace Raye::TaskManager
