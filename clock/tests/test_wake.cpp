// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wake.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace
{
const QTimeZone toronto("America/Toronto");

QDateTime local(int day, int h, int m = 0, int s = 0)
{
    return QDateTime(QDate(2026, 11, day), QTime(h, m, s), toronto);
}

Alarm daily(const char *id, int h, int m = 0)
{
    Alarm a;
    a.id = QString::fromLatin1(id);
    a.time = QTime(h, m);
    a.days = Alarms::kEveryDay;
    a.armed = local(18, 0);
    return a;
}
} // namespace

class TestWake : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void timers_do_not_wake_the_pc()
    {
        Countdown t;
        t.id = QStringLiteral("t");
        t.durationMs = 60000;
        t.endsAt = local(18, 6, 30);
        QVERIFY(t.running());
        QVERIFY(!Wake::nextAlarm({}, RingState(), local(18, 6), toronto).isValid());
        QCOMPARE(Wake::nextAlarm({daily("a", 7)}, RingState(), local(18, 6), toronto), local(18, 7));
    }

    void snooze_wakes_too()
    {
        RingState s;
        s.snoozed.insert(QStringLiteral("a"), local(18, 6, 10).toUTC());
        QCOMPARE(Wake::nextAlarm({daily("a", 7)}, s, local(18, 6), toronto), local(18, 6, 10));
    }

    void wakes_a_little_before()
    {
        QCOMPARE(Wake::wakeEpoch(local(18, 7), local(18, 6)), local(18, 7).toSecsSinceEpoch() - Wake::kLeadSecs);
        // too close to wait for: right now
        QCOMPARE(Wake::wakeEpoch(local(18, 7), local(18, 6, 59, 50)), local(18, 6, 59, 50).toSecsSinceEpoch());
    }

    void system_timer_unit()
    {
        const QByteArray unit = Wake::timerUnit(local(18, 7).toSecsSinceEpoch());
        QVERIFY(unit.contains("OnCalendar=2026-11-18 12:00:00 UTC"));
        QVERIFY(unit.contains("WakeSystem=true"));
        QVERIFY(unit.contains("Unit=raye-clock-wake.service"));
        QVERIFY(!unit.contains("Persistent=true"));
        QCOMPARE(Wake::unitName(1000), QStringLiteral("raye-clock-wake-1000.timer"));
    }

    void helper_refuses_odd_times()
    {
        const qint64 now = local(18, 6).toSecsSinceEpoch();
        QVERIFY(Wake::validEpoch(now + 3600, now));
        QVERIFY(Wake::validEpoch(now - 30, now));
        QVERIFY(!Wake::validEpoch(now - 3600, now));
        QVERIFY(!Wake::validEpoch(now + qint64(500) * 24 * 3600, now));
        bool ok = false;
        QCOMPARE(Wake::parseUid(QByteArray("1000"), &ok), 1000u);
        QVERIFY(ok);
        Wake::parseUid(QByteArray("10a0"), &ok);
        QVERIFY(!ok);
        Wake::parseUid(QByteArray(), &ok);
        QVERIFY(!ok);
    }

    void rtc_alarm_is_needed()
    {
        QTemporaryDir root;
        QVERIFY(!Wake::rtcCanWake(root.path()));
        QVERIFY(QDir().mkpath(root.path() + "/class/rtc/rtc0"));
        QVERIFY(!Wake::rtcCanWake(root.path()));
        QFile f(root.path() + "/class/rtc/rtc0/wakealarm");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
        QVERIFY(Wake::rtcCanWake(root.path()));
    }

    void helper_runs_only_when_the_time_changes()
    {
        QTemporaryDir dir;
        const QString last = dir.path() + "/wake";
        QStringList calls;
        auto fake = [&calls](const QStringList &args) {
            calls << args.join(' ');
            return true;
        };
        const QDateTime now = local(18, 6);
        const qint64 at = local(18, 7).toSecsSinceEpoch() - Wake::kLeadSecs;

        QCOMPARE(Wake::apply(local(18, 7), now, last, true, fake), Wake::Result::Set);
        QCOMPARE(calls, QStringList{"set " + QString::number(at)});
        QCOMPARE(Wake::apply(local(18, 7), now, last, true, fake), Wake::Result::Unchanged);
        QCOMPARE(calls.size(), 1);

        // the alarm is gone: the wake goes too, once
        QCOMPARE(Wake::apply({}, now, last, true, fake), Wake::Result::Cleared);
        QCOMPARE(calls.last(), QStringLiteral("clear"));
        QCOMPARE(Wake::apply({}, now, last, true, fake), Wake::Result::Unchanged);
        QCOMPARE(calls.size(), 2);

        // no RTC alarm on this PC: nothing is asked
        QCOMPARE(Wake::apply(local(18, 7), now, last, false, fake), Wake::Result::Unavailable);
        QCOMPARE(calls.size(), 2);

        // a refusal is tried again next time
        auto refuse = [&calls](const QStringList &args) {
            calls << args.join(' ');
            return false;
        };
        QCOMPARE(Wake::apply(local(18, 7), now, last, true, refuse), Wake::Result::Failed);
        QCOMPARE(Wake::apply(local(18, 7), now, last, true, fake), Wake::Result::Set);
        QCOMPARE(calls.size(), 4);
    }
};

QTEST_GUILESS_MAIN(TestWake)
#include "test_wake.moc"
