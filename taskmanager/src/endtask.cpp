// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "endtask.h"
#include "proctable.h"

#include <QFile>

#include <cerrno>
#include <csignal>

namespace Raye::TaskManager
{

EndTaskStep nextEndTaskStep(const EndTaskRequest &request, qint64 graceMs)
{
    if (!request.asked.isValid()) {
        return EndTaskStep::Gone;
    }
    // the number is free, or it has been handed to something else since
    // the row was drawn: either way there is nothing of ours to end
    if (!request.current.sameAs(request.asked)) {
        return EndTaskStep::Gone;
    }
    // a zombie has already run its course; it stays in the table until
    // its parent collects it, and no signal can hurry that along
    if (request.currentState == 'Z') {
        return EndTaskStep::Gone;
    }
    if (!request.termSent) {
        return EndTaskStep::Terminate;
    }
    if (request.userInsisted) {
        return EndTaskStep::Force;
    }
    if (request.msSinceTerm < graceMs) {
        return EndTaskStep::Wait;
    }
    return EndTaskStep::OfferForce;
}

ProcIdentity readIdentity(int pid, const QString &procRoot)
{
    ProcIdentity out;
    if (pid <= 0) {
        return out;
    }
    QFile file(procRoot + QStringLiteral("/%1/stat").arg(pid));
    if (!file.open(QIODevice::ReadOnly)) {
        return out;
    }
    const ProcStat stat = parseProcStat(file.readAll());
    if (!stat.valid || stat.pid != pid) {
        return out;
    }
    out.pid = stat.pid;
    out.startTime = stat.startTime;
    return out;
}

SignalOutcome sendSignalToIdentity(const ProcIdentity &identity, int signalNumber, const QString &procRoot)
{
    if (!identity.isValid()) {
        return SignalOutcome::Refused;
    }
    // read the start time again, as late as possible: the window between
    // this check and the kill() below is the only one left, and the
    // kernel does not reuse a number inside it
    if (!readIdentity(identity.pid, procRoot).sameAs(identity)) {
        return SignalOutcome::Refused;
    }
    if (::kill(identity.pid, signalNumber) == 0) {
        return SignalOutcome::Sent;
    }
    // ESRCH means it went away between the check and the call: that is
    // the race losing cleanly, not a failure to report to the user
    return errno == ESRCH ? SignalOutcome::Refused : SignalOutcome::Failed;
}

QStringList reniceAsRoot(const ProcIdentity &identity, int nice, const QString &renice)
{
    // fields after the name start at 3, so the start time (22) is ${20}
    static const QString script = QStringLiteral(
        "pid=$1 start=$2 nice=$3 renice=$4; s=$(cat /proc/$pid/stat 2>/dev/null) || exit 3; set -- ${s##*) }; "
        "[ \"${20}\" = \"$start\" ] || exit 3; exec \"$renice\" -n \"$nice\" -p \"$pid\"");
    return {QStringLiteral("/bin/sh"), QStringLiteral("-c"), script, QStringLiteral("sh"), QString::number(identity.pid),
            QString::number(identity.startTime), QString::number(nice), renice};
}

} // namespace Raye::TaskManager
