// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "efficiency.h"
#include "endtask.h"

#include <QFile>

#include <cerrno>
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace Raye::TaskManager
{

namespace
{
// ioprio_set(2) has no wrapper in glibc. The class sits in the top bits
// of the value; see Documentation/block/ioprio.rst.
constexpr int IoPrioWhoProcess = 1;
constexpr int IoPrioClassNone = 0;
constexpr int IoPrioClassIdle = 3;
constexpr int IoPrioClassShift = 13;

bool setIoPrio(int pid, int ioClass, int level)
{
    const int value = (ioClass << IoPrioClassShift) | level;
    return ::syscall(SYS_ioprio_set, IoPrioWhoProcess, pid, value) == 0;
}

// CAP_SYS_NICE is bit 23 of the capability set.
constexpr quint64 CapSysNiceBit = 1ull << 23;
}

bool isEfficiencyMode(const ProcStat &stat)
{
    return stat.valid && stat.policy == SchedIdle;
}

bool canLeaveEfficiencyMode(int rlimitNiceSoft, bool hasCap)
{
    return hasCap || rlimitNiceSoft >= RlimitNiceToLeaveIdle;
}

bool canSetNice(int target, int current, int rlimitNiceSoft, bool hasCap)
{
    if (hasCap) {
        return true;
    }
    // never raising a priority: lowering, or asking for what it already
    // has, is always allowed
    if (target >= current) {
        return true;
    }
    return target >= 20 - rlimitNiceSoft;
}

int rlimitNiceSoft()
{
    rlimit limit{};
    if (::getrlimit(RLIMIT_NICE, &limit) != 0) {
        return 0;
    }
    return int(limit.rlim_cur);
}

bool hasCapSysNice()
{
    QFile status(QStringLiteral("/proc/self/status"));
    if (!status.open(QIODevice::ReadOnly)) {
        return false;
    }
    for (const QByteArray &line : status.readAll().split('\n')) {
        if (!line.startsWith("CapEff:")) {
            continue;
        }
        bool ok = false;
        const quint64 caps = line.mid(7).trimmed().toULongLong(&ok, 16);
        return ok && (caps & CapSysNiceBit) != 0;
    }
    return false;
}

EfficiencyOutcome setEfficiencyMode(int pid, bool on)
{
    if (pid <= 0) {
        return EfficiencyOutcome::Refused;
    }

    if (!on) {
        // refused as a whole rather than half-done: see efficiency.h
        if (!canLeaveEfficiencyMode(rlimitNiceSoft(), hasCapSysNice())) {
            return EfficiencyOutcome::NotReversible;
        }
        sched_param param{};
        param.sched_priority = 0;
        if (::sched_setscheduler(pid, SCHED_OTHER, &param) != 0) {
            return errno == ESRCH ? EfficiencyOutcome::Refused : EfficiencyOutcome::NotReversible;
        }
        ::setpriority(PRIO_PROCESS, id_t(pid), 0);
        setIoPrio(pid, IoPrioClassNone, 0);
        return EfficiencyOutcome::Applied;
    }

    // the I/O class first: it is the half that can always be undone, so
    // a failure of the other one leaves the lesser mark
    const bool io = setIoPrio(pid, IoPrioClassIdle, 0);
    sched_param param{};
    param.sched_priority = 0;
    if (::sched_setscheduler(pid, SCHED_IDLE, &param) != 0) {
        if (errno == ESRCH) {
            return EfficiencyOutcome::Refused;
        }
        return io ? EfficiencyOutcome::IoOnly : EfficiencyOutcome::Refused;
    }
    ::setpriority(PRIO_PROCESS, id_t(pid), EfficiencyNice);
    return io ? EfficiencyOutcome::Applied : EfficiencyOutcome::IoOnly;
}

EfficiencyOutcome setEfficiencyModeChecked(const ProcStat &asked, bool on, const QString &procRoot)
{
    if (!asked.valid) {
        return EfficiencyOutcome::Refused;
    }
    const ProcIdentity identity{asked.pid, asked.startTime};
    // the same guard as a signal: a row drawn a minute ago may name a
    // number that belongs to something else now, and slowing the wrong
    // process to a crawl is not a harmless mistake
    if (!readIdentity(asked.pid, procRoot).sameAs(identity)) {
        return EfficiencyOutcome::Refused;
    }
    return setEfficiencyMode(asked.pid, on);
}

} // namespace Raye::TaskManager
