// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Ending a task: the order of the two signals, and the guard that keeps
// the second one off an innocent process.

#include "endtask.h"

#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

#include <csignal>

using namespace Raye::TaskManager;

class TestEndTask : public QObject
{
    Q_OBJECT

private:
    static EndTaskRequest alive(bool termSent = false, qint64 since = 0, bool insisted = false)
    {
        EndTaskRequest request;
        request.asked = ProcIdentity{4242, 900};
        request.current = ProcIdentity{4242, 900};
        request.currentState = 'S';
        request.termSent = termSent;
        request.msSinceTerm = since;
        request.userInsisted = insisted;
        return request;
    }

private Q_SLOTS:
    void theTerminationComesFirst()
    {
        QCOMPARE(nextEndTaskStep(alive()), EndTaskStep::Terminate);
    }

    void theForceWaitsAndThenAsks()
    {
        QCOMPARE(nextEndTaskStep(alive(true, 1)), EndTaskStep::Wait);
        QCOMPARE(nextEndTaskStep(alive(true, EndTaskGraceMs - 1)), EndTaskStep::Wait);
        QCOMPARE(nextEndTaskStep(alive(true, EndTaskGraceMs)), EndTaskStep::OfferForce);
        // the grace alone never forces: the user has to ask again
        QCOMPARE(nextEndTaskStep(alive(true, 10 * EndTaskGraceMs)), EndTaskStep::OfferForce);
        QCOMPARE(nextEndTaskStep(alive(true, EndTaskGraceMs, true)), EndTaskStep::Force);
    }

    // Same number, a different process. Comparing numbers alone, this
    // is "still there, force it" -- and the thing forced is whatever was
    // given that number in the meantime.
    void aRecycledNumberEndsNothing()
    {
        EndTaskRequest request = alive(true, 10 * EndTaskGraceMs, true);
        request.current.startTime = request.asked.startTime + 1;
        QCOMPARE(nextEndTaskStep(request), EndTaskStep::Gone);
    }

    void aNumberNobodyHasEndsNothing()
    {
        EndTaskRequest request = alive();
        request.current = ProcIdentity{};
        QCOMPARE(nextEndTaskStep(request), EndTaskStep::Gone);

        EndTaskRequest nothingAsked = alive();
        nothingAsked.asked = ProcIdentity{};
        QCOMPARE(nextEndTaskStep(nothingAsked), EndTaskStep::Gone);
    }

    // a zombie has already ended; it waits for its parent, and no signal
    // shortens that wait
    void aZombieHasAlreadyEnded()
    {
        EndTaskRequest request = alive();
        request.currentState = 'Z';
        QCOMPARE(nextEndTaskStep(request), EndTaskStep::Gone);
    }

    // the same guard, against a real process rather than a structure
    void noSignalGoesToTheWrongStartTime()
    {
        QProcess sleeper;
        sleeper.start(QStringLiteral("sleep"), {QStringLiteral("30")});
        QVERIFY2(sleeper.waitForStarted(5000), "sleep(1) is needed by this test");
        const int pid = int(sleeper.processId());

        const ProcIdentity real = readIdentity(pid, QStringLiteral("/proc"));
        QCOMPARE(real.pid, pid);
        QVERIFY(real.startTime > 0);

        const ProcIdentity stale{pid, real.startTime + 1};
        QCOMPARE(sendSignalToIdentity(stale, SIGTERM, QStringLiteral("/proc")), SignalOutcome::Refused);
        // refused means refused: it is still running
        QCOMPARE(sleeper.state(), QProcess::Running);

        QCOMPARE(sendSignalToIdentity(real, SIGTERM, QStringLiteral("/proc")), SignalOutcome::Sent);
        QVERIFY(sleeper.waitForFinished(5000));
    }

    void anEmptyProcTreeRefusesEverything()
    {
        QTemporaryDir empty;
        QVERIFY(empty.isValid());
        QCOMPARE(readIdentity(1, empty.path()).isValid(), false);
        QCOMPARE(sendSignalToIdentity(ProcIdentity{1, 1}, SIGTERM, empty.path()), SignalOutcome::Refused);
        QCOMPARE(sendSignalToIdentity(ProcIdentity{}, SIGTERM, QStringLiteral("/proc")), SignalOutcome::Refused);
    }

    // the command pkexec runs, run here without it and with a stand-in
    // renice that writes down how it was called
    void reniceChecksTheStartTime()
    {
        QTemporaryDir tmp;
        const QString fake = tmp.filePath(QStringLiteral("renice"));
        const QString log = tmp.filePath(QStringLiteral("called"));
        QFile f(fake);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QStringLiteral("#!/bin/sh\necho \"$*\" >> %1\n").arg(log).toUtf8());
        f.close();
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

        QProcess sleeper;
        sleeper.start(QStringLiteral("sleep"), {QStringLiteral("30")});
        QVERIFY2(sleeper.waitForStarted(5000), "sleep(1) is needed by this test");
        const int pid = int(sleeper.processId());
        const ProcIdentity real = readIdentity(pid, QStringLiteral("/proc"));
        QVERIFY(real.isValid());
        auto run = [](const QStringList &cmd) {
            QProcess p;
            p.start(cmd.first(), cmd.mid(1));
            p.waitForFinished(5000);
            return p.exitCode();
        };
        auto called = [&log]() {
            QFile l(log);
            return l.open(QIODevice::ReadOnly) ? QString::fromUtf8(l.readAll()).trimmed() : QString();
        };
        QCOMPARE(run(reniceAsRoot({pid, real.startTime + 1}, -5, fake)), 3);
        QCOMPARE(called(), QString());
        QCOMPARE(run(reniceAsRoot(real, -5, fake)), 0);
        QCOMPARE(called(), QStringLiteral("-n -5 -p %1").arg(pid));
        sleeper.kill();
        sleeper.waitForFinished();
    }
};

QTEST_GUILESS_MAIN(TestEndTask)
#include "test_endtask.moc"
