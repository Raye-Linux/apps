// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// What the Processes tab reads out of /proc, and the three ways it has
// been got wrong elsewhere:
//
//   1. splitting /proc/<pid>/stat on spaces, which a process name
//      containing a bracket and a space turns into another process;
//   2. summing the per-processor lines of /proc/stat, which counts every
//      tick once per processor;
//   3. showing utime+stime as if it were a rate, which is a lifetime
//      average that never moves.

#include "efficiency.h"
#include "proctable.h"

#include <QFile>
#include <QTest>

#include <unistd.h>

using namespace Raye::TaskManager;

namespace
{
// /proc/1/stat of this machine, taken as it stands. 50 fields follow the
// name, so field 41 (the scheduling class) is really there.
const QByteArray realLine =
    "1 (systemd) S 0 1 1 0 -1 4194560 167828 1058301254 418 105384 995 662 11378559 1055055 20 0 1 0 38 "
    "24776704 2503 18446744073709551615 94903280427008 94903280482853 140722918341728 0 0 0 671173123 "
    "4096 1260 1 0 0 17 21 0 0 0 0 0 94903280523856 94903280529424 94903735390208 140722918342526 "
    "140722918342537 140722918342537 140722918342637 0\n";
}

class TestProcTable : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fullNameOfACutName()
    {
        QCOMPARE(displayName(QStringLiteral("raye-taskmanage"), QByteArray("/usr/bin/raye-taskmanager\0-x\0", 29)),
                 QStringLiteral("raye-taskmanager"));
        // a short name is never looked up
        QCOMPARE(displayName(QStringLiteral("bash"), QByteArray("/bin/zsh\0", 9)), QStringLiteral("bash"));
        // a command line that does not match keeps the kernel's name
        QCOMPARE(displayName(QStringLiteral("kworker/u64:2-e"), QByteArray()), QStringLiteral("kworker/u64:2-e"));
        QCOMPARE(displayName(QStringLiteral("Web Content Pro"), QByteArray("/usr/lib/firefox/firefox\0", 25)),
                 QStringLiteral("Web Content Pro"));
    }

    void readsAPlainLine()
    {
        const ProcStat stat = parseProcStat(realLine);
        QVERIFY(stat.valid);
        QCOMPARE(stat.pid, 1);
        QCOMPARE(stat.comm, QStringLiteral("systemd"));
        QCOMPARE(stat.state, 'S');
        QCOMPARE(stat.ppid, 0);
        QCOMPARE(stat.utime, 995ull);
        QCOMPARE(stat.stime, 662ull);
        QCOMPARE(stat.nice, 0);
        QCOMPARE(stat.threads, 1);
        QCOMPARE(stat.flags, 4194560ull);
        QVERIFY(!stat.isKernelThread());
        // kthreadd, as this machine prints it: PF_KTHREAD is set
        const ProcStat kernel = parseProcStat("2 (kthreadd) S 0 0 0 0 -1 2129984 0 0 0 0 0 0 0 0 20 0 1 0 2 0 0 "
                                              "18446744073709551615 0 0 0 0 0 0 0 2147483647 0 1 0 0 0 0 0 0 0 0\n");
        QVERIFY(kernel.valid);
        QVERIFY(kernel.isKernelThread());
        QCOMPARE(stat.startTime, 38ull);
        QCOMPARE(stat.rssPages, 2503);
        QCOMPARE(stat.policy, SchedOther);
    }

    // The line the kernel writes for a process called
    // "evil) S 1 1 1 0 -1 0" -- a name it truncates to 15 bytes but does
    // not escape. Read from the FIRST bracket, this says state R and
    // parent 1; read from the last, it says what it really is.
    void aNameMayHoldABracket()
    {
        QByteArray line = realLine;
        line.replace("(systemd)", "(evil) S 1 1 1 0 -1 0)");
        const ProcStat stat = parseProcStat(line);
        QVERIFY(stat.valid);
        QCOMPARE(stat.comm, QStringLiteral("evil) S 1 1 1 0 -1 0"));
        // the real fields, the ones after the LAST bracket
        QCOMPARE(stat.state, 'S');
        QCOMPARE(stat.ppid, 0);
        QCOMPARE(stat.utime, 995ull);
        QCOMPARE(stat.startTime, 38ull);
    }

    void refusesWhatIsNotALine_data()
    {
        QTest::addColumn<QByteArray>("line");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("no bracket") << QByteArray("1 systemd S 0 1\n");
        QTest::newRow("no pid") << QByteArray("(systemd) S 0 1 1\n");
        QTest::newRow("words where numbers go") << QByteArray("1 (systemd) S nought one\n");
        QTest::newRow("nothing after the name") << QByteArray("1 (systemd)\n");
    }

    void refusesWhatIsNotALine()
    {
        QFETCH(QByteArray, line);
        QCOMPARE(parseProcStat(line).valid, false);
    }

    // the same reading, against the kernel rather than a fixture
    void readsThisProcess()
    {
        QFile file(QStringLiteral("/proc/self/stat"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const ProcStat stat = parseProcStat(file.readAll());
        QVERIFY(stat.valid);
        QCOMPARE(stat.pid, int(::getpid()));
        QCOMPARE(stat.ppid, int(::getppid()));
        QVERIFY(stat.startTime > 0);
        QVERIFY(stat.rssPages > 0);
    }

    // "cpu0" is not "cpu ". Adding the per-processor lines in would
    // give four times the truth on a four-processor machine, and the
    // processor column would then read a quarter of what it should.
    void totalJiffiesTakesTheSummaryLineAlone()
    {
        const QByteArray text = "cpu  10 20 30 40\n"
                                "cpu0 5 10 15 20\n"
                                "cpu1 5 10 15 20\n"
                                "intr 99\n";
        QCOMPARE(parseTotalJiffies(text), 100ull);
        // a tree with no summary line at all: 0, and not the figure of
        // the first processor, which a startsWith("cpu") would take
        QCOMPARE(parseTotalJiffies("cpu0 5 10 15 20\ncpu1 5 10 15 20\n"), 0ull);
        QCOMPARE(parseTotalJiffies("intr 99\n"), 0ull);
        QCOMPARE(parseTotalJiffies(QByteArray()), 0ull);
    }

    // 100 % is the whole machine: 100 jiffies of work while the machine
    // spent 800 is one processor out of eight, 12.5 %.
    void processorShareIsADifference()
    {
        ProcStat before = parseProcStat(realLine);
        ProcStat after = before;
        after.utime = before.utime + 100;

        QCOMPARE(cpuPercent(before, after, 1000, 1800), 12.5);
        // a sample that has not moved is 0, not the lifetime average
        QCOMPARE(cpuPercent(before, before, 1000, 1800), 0.0);
        // a machine clock that has not advanced: no division by zero
        QCOMPARE(cpuPercent(before, after, 1000, 1000), 0.0);
        // and never a negative share from a counter read out of order
        QCOMPARE(cpuPercent(after, before, 1000, 1800), 0.0);
    }

    // Same number, different start time: a new process has been given
    // the number of the one being watched, and its counters are not the
    // continuation of the ones held.
    void aRecycledNumberShowsNoShare()
    {
        ProcStat before = parseProcStat(realLine);
        ProcStat after = before;
        after.startTime = before.startTime + 1;
        after.utime = before.utime + 100;
        QCOMPARE(cpuPercent(before, after, 1000, 1800), 0.0);
    }

    void memoryIsPagesTimesThePageSize()
    {
        const ProcStat stat = parseProcStat(realLine);
        QCOMPARE(residentBytes(stat, 4096), 2503LL * 4096);
        QCOMPARE(residentBytes(stat, 0), 0LL);
        QCOMPARE(residentBytes(ProcStat{}, 4096), 0LL);
    }

    void theEfficiencyClassIsReadFromFieldFortyOne()
    {
        ProcStat stat = parseProcStat(realLine);
        QCOMPARE(isEfficiencyMode(stat), false);
        stat.policy = SchedIdle;
        QVERIFY(isEfficiencyMode(stat));
        QCOMPARE(isEfficiencyMode(ProcStat{}), false);
    }
};

QTEST_GUILESS_MAIN(TestProcTable)
#include "test_proctable.moc"
