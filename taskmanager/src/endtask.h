// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// "End task": SIGTERM first, SIGKILL only after a wait AND only when the
// user asks a second time.
//
// Why not SIGKILL straight away, which is what a one-click "kill" would
// do: a program killed outright never runs its save-on-exit, so the
// letter being written is lost. SIGTERM is the request an editor answers
// by saving; the force is the second step, named as such in the window.
//
// A process number is reused. Linux hands out numbers up to
// /proc/sys/kernel/pid_max and then wraps round, so between the moment a
// row is drawn and the moment its button is pressed, that number may
// belong to something else entirely -- and on a busy machine that is not
// a rare event, it is a matter of minutes. Field 22 of
// /proc/<pid>/stat, the start time, tells one from the other: it is set
// once and never changes. Every signal this file sends is checked against
// it, so a stale row ends nothing rather than ending the wrong thing.

#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace Raye::TaskManager
{

// How long the program is given to answer SIGTERM before the window
// offers to force it. Windows waits about this long before saying an
// application is not responding.
constexpr qint64 EndTaskGraceMs = 5000;

// A process AND the moment it started: the pair is unique for the life
// of the machine, where the number alone is not.
struct ProcIdentity {
    int pid = 0;
    qulonglong startTime = 0; // field 22 of /proc/<pid>/stat

    bool isValid() const
    {
        return pid > 0;
    }
    bool sameAs(const ProcIdentity &other) const
    {
        return pid == other.pid && startTime == other.startTime && isValid();
    }
};

enum class EndTaskStep {
    Gone, // nothing left to end: it has gone, or another process wears its number
    Terminate, // send SIGTERM
    Wait, // SIGTERM sent, still within the grace
    OfferForce, // the grace is over and it is still there: ask before forcing
    Force, // the user asked a second time: SIGKILL
};

struct EndTaskRequest {
    ProcIdentity asked; // the row the user pressed
    ProcIdentity current; // what wears that number now; invalid when gone
    char currentState = '\0'; // field 3; 'Z' is already dead
    bool termSent = false;
    qint64 msSinceTerm = 0;
    bool userInsisted = false; // the force was asked for, in so many words
};

// The one step to take now. A pure function: the caller reads /proc, this
// decides, and the test can walk the whole sequence without a process.
EndTaskStep nextEndTaskStep(const EndTaskRequest &request, qint64 graceMs = EndTaskGraceMs);

enum class SignalOutcome {
    Sent,
    Refused, // the number belongs to another process, or to none
    Failed, // the kernel refused the signal (not ours to signal)
};

// Sends signalNumber to identity, and only to it: the start time is read
// again from /proc and compared first.
//
// procRoot is "/proc" outside the tests; they hand it a tree of their
// own.
SignalOutcome sendSignalToIdentity(const ProcIdentity &identity, int signalNumber, const QString &procRoot);

// Reads /proc/<pid>/stat and returns the pair for that number.
ProcIdentity readIdentity(int pid, const QString &procRoot);

// What pkexec runs to set a priority as root: the start time is read again
// after the password, so a number taken over meanwhile is left alone.
// Exits 3 when the process is no longer the one asked for.
QStringList reniceAsRoot(const ProcIdentity &identity, int nice, const QString &renice = QStringLiteral("/usr/bin/renice"));

} // namespace Raye::TaskManager
