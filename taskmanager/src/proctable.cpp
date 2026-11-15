// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "proctable.h"

#include <QList>

namespace Raye::TaskManager
{

namespace
{
// The fields after the name start at field 3, so field N is at N - 3.
constexpr int afterComm = 3;

qulonglong field(const QList<QByteArray> &rest, int number, bool *ok)
{
    const int index = number - afterComm;
    if (index < 0 || index >= rest.size()) {
        *ok = false;
        return 0;
    }
    bool parsed = false;
    const qulonglong value = rest.at(index).toULongLong(&parsed);
    if (!parsed) {
        *ok = false;
    }
    return value;
}

qlonglong signedField(const QList<QByteArray> &rest, int number, bool *ok)
{
    const int index = number - afterComm;
    if (index < 0 || index >= rest.size()) {
        *ok = false;
        return 0;
    }
    bool parsed = false;
    const qlonglong value = rest.at(index).toLongLong(&parsed);
    if (!parsed) {
        *ok = false;
    }
    return value;
}
}

ProcStat parseProcStat(const QByteArray &line)
{
    ProcStat out;

    const int open = line.indexOf('(');
    // the LAST bracket, not the first one after it: the name may hold
    // brackets of its own, and the kernel does not escape them
    const int close = line.lastIndexOf(')');
    if (open < 1 || close < open + 1 || close + 2 >= line.size()) {
        return out;
    }

    bool ok = true;
    const qlonglong pid = line.left(open).trimmed().toLongLong(&ok);
    if (!ok || pid <= 0) {
        return out;
    }

    const QList<QByteArray> rest = line.mid(close + 2).simplified().split(' ');
    // 41 fields are wanted; a kernel that stops earlier is read as far as
    // it goes, which is why every field() call reports for itself
    if (rest.isEmpty() || rest.first().isEmpty()) {
        return out;
    }

    out.pid = int(pid);
    out.comm = QString::fromUtf8(line.mid(open + 1, close - open - 1));
    out.state = rest.at(0).at(0);
    out.ppid = int(field(rest, 4, &ok));
    out.flags = field(rest, 9, &ok);
    out.utime = field(rest, 14, &ok);
    out.stime = field(rest, 15, &ok);
    out.nice = int(signedField(rest, 19, &ok));
    out.threads = int(field(rest, 20, &ok));
    out.startTime = field(rest, 22, &ok);
    out.rssPages = signedField(rest, 24, &ok);
    // field 41 arrived in Linux 2.6.18; a line without it is not an
    // error, the process simply is not in the efficiency class
    bool policyOk = true;
    const qulonglong policy = field(rest, 41, &policyOk);
    out.policy = policyOk ? int(policy) : SchedOther;

    out.valid = ok;
    if (!out.valid) {
        return ProcStat{};
    }
    return out;
}

qulonglong parseTotalJiffies(const QByteArray &procStat)
{
    for (const QByteArray &line : procStat.split('\n')) {
        // "cpu " and not "cpu": "cpu0" must not be taken for the total
        if (!line.startsWith("cpu ")) {
            continue;
        }
        qulonglong total = 0;
        const QList<QByteArray> parts = line.simplified().split(' ');
        for (int i = 1; i < parts.size(); ++i) {
            bool ok = false;
            const qulonglong value = parts.at(i).toULongLong(&ok);
            if (!ok) {
                return 0;
            }
            total += value;
        }
        return total;
    }
    return 0;
}

double cpuPercent(const ProcStat &before, const ProcStat &after, qulonglong totalBefore, qulonglong totalAfter)
{
    if (!before.valid || !after.valid) {
        return 0.0;
    }
    // a different process wearing the same number: its jiffies are not
    // the continuation of the ones held
    if (before.pid != after.pid || before.startTime != after.startTime) {
        return 0.0;
    }
    if (totalAfter <= totalBefore || after.cpuJiffies() < before.cpuJiffies()) {
        return 0.0;
    }
    const double used = double(after.cpuJiffies() - before.cpuJiffies());
    const double elapsed = double(totalAfter - totalBefore);
    return used / elapsed * 100.0;
}

QString displayName(const QString &comm, const QByteArray &cmdline)
{
    if (comm.size() < 15) {
        return comm;
    }
    const QByteArray first = cmdline.left(cmdline.indexOf('\0'));
    const QString program = QString::fromUtf8(first.mid(first.lastIndexOf('/') + 1));
    return program.startsWith(comm) ? program : comm;
}

qlonglong residentBytes(const ProcStat &stat, int pageSize)
{
    if (!stat.valid || stat.rssPages <= 0 || pageSize <= 0) {
        return 0;
    }
    return stat.rssPages * pageSize;
}

} // namespace Raye::TaskManager
