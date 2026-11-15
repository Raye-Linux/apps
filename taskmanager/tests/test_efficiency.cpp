// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The efficiency mode, and the one-way door the window has to own up to.
// The boundary at RLIMIT_NICE 20 was measured on this desktop, one unit
// at a time: 18 and 19 refused the way back, 20 and 21 allowed it.

#include "efficiency.h"
#include "proctable.h"

#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

using namespace Raye::TaskManager;

namespace
{
int policyOf(int pid)
{
    QFile file(QStringLiteral("/proc/%1/stat").arg(pid));
    if (!file.open(QIODevice::ReadOnly)) {
        return -1;
    }
    const ProcStat stat = parseProcStat(file.readAll());
    return stat.valid ? stat.policy : -1;
}
}

class TestEfficiency : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // The boundary. Below 20, an owner cannot take their own process
    // out of the idle class, so the window must not draw a switch.
    void leavingNeedsRoomAboveOrTheCapability()
    {
        QCOMPARE(canLeaveEfficiencyMode(0, false), false);
        QCOMPARE(canLeaveEfficiencyMode(19, false), false);
        QCOMPARE(canLeaveEfficiencyMode(20, false), true);
        QCOMPARE(canLeaveEfficiencyMode(21, false), true);
        // the capability settles it whatever the limit
        QCOMPARE(canLeaveEfficiencyMode(0, true), true);
    }

    void loweringIsAlwaysAllowedAndRaisingIsNot()
    {
        // 0 -> 19: lowering
        QCOMPARE(canSetNice(19, 0, 0, false), true);
        // 19 -> 0: raising, and the default limit leaves no room
        QCOMPARE(canSetNice(0, 19, 0, false), false);
        // the value it already has: always
        QCOMPARE(canSetNice(19, 19, 0, false), true);
        // room above: 20 - 20 = 0, so nice 0 is within reach
        QCOMPARE(canSetNice(0, 19, 20, false), true);
        QCOMPARE(canSetNice(-5, 19, 20, false), false);
        QCOMPARE(canSetNice(-5, 19, 25, false), true);
        QCOMPARE(canSetNice(-20, 19, 0, true), true);
    }

    void thisProcessAnswersForItself()
    {
        QVERIFY(rlimitNiceSoft() >= 0);
        // whatever the answer, the two agree with the pure rule
        QCOMPARE(canLeaveEfficiencyMode(rlimitNiceSoft(), hasCapSysNice()),
                 hasCapSysNice() || rlimitNiceSoft() >= RlimitNiceToLeaveIdle);
    }

    // against a real process: the class in field 41 really changes
    void aProcessReallyEntersTheIdleClass()
    {
        QProcess sleeper;
        sleeper.start(QStringLiteral("sleep"), {QStringLiteral("30")});
        QVERIFY2(sleeper.waitForStarted(5000), "sleep(1) is needed by this test");
        const int pid = int(sleeper.processId());
        QCOMPARE(policyOf(pid), SchedOther);

        const EfficiencyOutcome on = setEfficiencyMode(pid, true);
        QVERIFY(on == EfficiencyOutcome::Applied || on == EfficiencyOutcome::IoOnly);
        QCOMPARE(policyOf(pid), SchedIdle);

        // and the way back is refused, or taken, exactly as the rule says
        const EfficiencyOutcome off = setEfficiencyMode(pid, false);
        if (canLeaveEfficiencyMode(rlimitNiceSoft(), hasCapSysNice())) {
            QCOMPARE(off, EfficiencyOutcome::Applied);
            QCOMPARE(policyOf(pid), SchedOther);
        } else {
            QCOMPARE(off, EfficiencyOutcome::NotReversible);
            // refused as a whole: nothing was half-undone
            QCOMPARE(policyOf(pid), SchedIdle);
        }

        sleeper.kill();
        QVERIFY(sleeper.waitForFinished(5000));
    }

    // The same guard as a signal. Slowing the wrong process to a
    // crawl is not a harmless mistake: it looks like a machine fault.
    void aRecycledNumberIsNotSlowedDown()
    {
        QProcess sleeper;
        sleeper.start(QStringLiteral("sleep"), {QStringLiteral("30")});
        QVERIFY2(sleeper.waitForStarted(5000), "sleep(1) is needed by this test");
        const int pid = int(sleeper.processId());

        QFile file(QStringLiteral("/proc/%1/stat").arg(pid));
        QVERIFY(file.open(QIODevice::ReadOnly));
        ProcStat stat = parseProcStat(file.readAll());
        QVERIFY(stat.valid);

        ProcStat stale = stat;
        stale.startTime += 1;
        QCOMPARE(setEfficiencyModeChecked(stale, true, QStringLiteral("/proc")), EfficiencyOutcome::Refused);
        QCOMPARE(policyOf(pid), SchedOther);

        QVERIFY(setEfficiencyModeChecked(stat, true, QStringLiteral("/proc")) != EfficiencyOutcome::Refused);
        QCOMPARE(policyOf(pid), SchedIdle);

        sleeper.kill();
        QVERIFY(sleeper.waitForFinished(5000));
    }

    void nothingIsDoneWithoutAProcess()
    {
        QTemporaryDir empty;
        QVERIFY(empty.isValid());
        QCOMPARE(setEfficiencyMode(0, true), EfficiencyOutcome::Refused);
        QCOMPARE(setEfficiencyModeChecked(ProcStat{}, true, QStringLiteral("/proc")), EfficiencyOutcome::Refused);
        ProcStat invented;
        invented.valid = true;
        invented.pid = 4242;
        invented.startTime = 7;
        QCOMPARE(setEfficiencyModeChecked(invented, true, empty.path()), EfficiencyOutcome::Refused);
    }
};

QTEST_GUILESS_MAIN(TestEfficiency)
#include "test_efficiency.moc"
