// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "alarms.h"

#include <QTest>

namespace
{
const QTimeZone toronto("America/Toronto");

QDateTime local(int month, int day, int h, int m = 0)
{
    return QDateTime(QDate(2026, month, day), QTime(h, m), toronto);
}

Alarm alarm(int h, int m, int days = 0)
{
    Alarm a;
    a.id = "a";
    a.time = QTime(h, m);
    a.days = days;
    a.armed = local(1, 1, 0);
    return a;
}

constexpr int Mon = 1, Tue = 2, Sat = 1 << 5, Sun = 1 << 6;
} // namespace

class TestAlarms : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void once_rings_today_then_tomorrow()
    {
        const Alarm a = alarm(7, 30);
        QCOMPARE(Alarms::nextAfter(a, local(11, 18, 6, 0), toronto), local(11, 18, 7, 30));
        QCOMPARE(Alarms::nextAfter(a, local(11, 18, 7, 30), toronto), local(11, 19, 7, 30));
    }

    void repeat_days_skip_others()
    {
        // 2026-11-18 is a Wednesday
        const Alarm a = alarm(8, 0, Mon | Tue);
        QCOMPARE(Alarms::nextAfter(a, local(11, 18, 6, 0), toronto), local(11, 23, 8, 0));
        QCOMPARE(Alarms::nextAfter(a, local(11, 23, 8, 0), toronto), local(11, 24, 8, 0));
        const Alarm weekend = alarm(9, 0, Sat | Sun);
        QCOMPARE(Alarms::nextAfter(weekend, local(11, 21, 9, 0), toronto), local(11, 22, 9, 0));
        QCOMPARE(Alarms::nextAfter(weekend, local(11, 22, 9, 1), toronto), local(11, 28, 9, 0));
    }

    void every_day_mask()
    {
        for (int d = 1; d <= 7; ++d)
            QVERIFY(Alarms::ringsOn(Alarms::kEveryDay, d));
        QVERIFY(Alarms::ringsOn(Alarms::kWeekdays, 5));
        QVERIFY(!Alarms::ringsOn(Alarms::kWeekdays, 6));
        QVERIFY(Alarms::ringsOn(0, 3));
    }

    void switched_off_never_rings()
    {
        Alarm a = alarm(7, 0, Alarms::kEveryDay);
        a.enabled = false;
        QVERIFY(!Alarms::nextAfter(a, local(11, 18, 6, 0), toronto).isValid());
    }

    void spring_forward_rings_when_clock_jumps()
    {
        // 2026-03-08: 2:00 becomes 3:00 in Toronto, 2:30 never happens
        const Alarm a = alarm(2, 30, Alarms::kEveryDay);
        const QDateTime ring = Alarms::nextAfter(a, local(3, 8, 0, 0), toronto);
        QCOMPARE(ring.toUTC(), QDateTime(QDate(2026, 3, 8), QTime(7, 0), QTimeZone::UTC));
        QCOMPARE(ring.toTimeZone(toronto).time(), QTime(3, 0));
        // and the day after, back at 2:30
        QCOMPARE(Alarms::nextAfter(a, ring, toronto), local(3, 9, 2, 30));
    }

    void spring_forward_keeps_wall_time_after()
    {
        // 7:00 the day before and the day of the change: 24 h, then 23 h apart
        const Alarm a = alarm(7, 0, Alarms::kEveryDay);
        const QDateTime sat = Alarms::nextAfter(a, local(3, 7, 0, 0), toronto);
        const QDateTime sun = Alarms::nextAfter(a, sat, toronto);
        QCOMPARE(sat.secsTo(sun), 23 * 3600);
        QCOMPARE(sun.toTimeZone(toronto).time(), QTime(7, 0));
    }

    void fall_back_rings_once_at_first_pass()
    {
        // 2026-11-01: 2:00 goes back to 1:00, 1:30 happens twice
        const Alarm a = alarm(1, 30, Alarms::kEveryDay);
        const QDateTime ring = Alarms::nextAfter(a, local(11, 1, 0, 0), toronto);
        QCOMPARE(ring.toUTC(), QDateTime(QDate(2026, 11, 1), QTime(5, 30), QTimeZone::UTC));
        const QDateTime after = Alarms::nextAfter(a, ring, toronto);
        QCOMPARE(after.toTimeZone(toronto).date(), QDate(2026, 11, 2));
        QCOMPARE(after.toTimeZone(toronto).time(), QTime(1, 30));
    }

    void europe_too()
    {
        const QTimeZone paris("Europe/Paris");
        Alarm a = alarm(2, 15, Alarms::kEveryDay);
        // 2026-03-29: 2:00 becomes 3:00 in Paris
        const QDateTime ring = Alarms::nextAfter(a, QDateTime(QDate(2026, 3, 29), QTime(0, 0), paris), paris);
        QCOMPARE(ring.toTimeZone(paris).time(), QTime(3, 0));
    }

    void last_between_takes_latest()
    {
        const Alarm a = alarm(7, 0, Alarms::kEveryDay);
        QCOMPARE(Alarms::lastBetween(a, local(11, 15, 12, 0), local(11, 18, 8, 0), toronto), local(11, 18, 7, 0));
        QVERIFY(!Alarms::lastBetween(a, local(11, 18, 7, 0), local(11, 18, 8, 0), toronto).isValid());
    }

    void last_between_once_is_its_one_time()
    {
        // armed long ago, never rang: the one time is the first after arming
        const Alarm a = alarm(7, 0);
        QCOMPARE(Alarms::lastBetween(a, local(10, 1, 12, 0), local(11, 18, 8, 0), toronto), local(10, 2, 7, 0));
    }

    void json_round_trip()
    {
        Alarm a = alarm(6, 45, Mon | Sun);
        a.name = "Gym";
        a.sound = "chimes";
        a.snoozeMinutes = 5;
        a.enabled = false;
        a.armed = local(11, 18, 6, 0).toUTC();
        const QList<Alarm> back = Alarms::fromJson(Alarms::toJson({a}));
        QCOMPARE(back.size(), 1);
        QCOMPARE(back.first(), a);
    }

    void json_drops_broken_entries()
    {
        const QByteArray data = R"({"alarms":[{"id":"x","time":"25:00"},{"time":"07:00"},{"id":"ok","time":"07:00","snooze":500}]})";
        const QList<Alarm> back = Alarms::fromJson(data);
        QCOMPARE(back.size(), 1);
        QCOMPARE(back.first().snoozeMinutes, 60);
        QVERIFY(back.first().enabled);
    }
};

QTEST_GUILESS_MAIN(TestAlarms)
#include "test_alarms.moc"
