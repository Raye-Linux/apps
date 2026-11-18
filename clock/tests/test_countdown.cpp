// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "countdown.h"

#include <QTest>
#include <QTimeZone>

namespace
{
const QDateTime t0(QDate(2026, 11, 18), QTime(10, 0), QTimeZone::UTC);

QDateTime at(qint64 ms)
{
    return t0.addMSecs(ms);
}

Countdown fiveMinutes()
{
    Countdown c;
    c.id = "t";
    c.setDuration(5 * 60 * 1000);
    return c;
}
} // namespace

class TestCountdown : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void runs_pauses_and_resumes()
    {
        Countdown c = fiveMinutes();
        QCOMPARE(c.remainingMs(t0), 300000);
        c.start(t0);
        QVERIFY(c.running());
        QCOMPARE(c.remainingMs(at(60000)), 240000);
        c.pause(at(60000));
        QVERIFY(!c.running());
        // standing still for an hour changes nothing
        QCOMPARE(c.remainingMs(at(3600000)), 240000);
        c.start(at(3600000));
        QCOMPARE(c.endsAt, at(3600000 + 240000));
        QCOMPARE(c.fraction(at(3600000 + 120000)), 0.4);
    }

    void over_and_restart()
    {
        Countdown c = fiveMinutes();
        c.start(t0);
        QVERIFY(!c.over(at(299999)));
        QVERIFY(c.over(at(300000)));
        QCOMPARE(c.remainingMs(at(400000)), 0);
        c.restart(at(400000));
        QCOMPARE(c.remainingMs(at(400000)), 300000);
        c.reset();
        QVERIFY(!c.running());
        QCOMPARE(c.leftMs, 300000);
    }

    void start_when_empty_starts_over()
    {
        Countdown c = fiveMinutes();
        c.leftMs = 0;
        c.start(t0);
        QCOMPARE(c.endsAt, at(300000));
    }

    void duration_is_bounded()
    {
        Countdown c;
        c.setDuration(0);
        QCOMPARE(c.durationMs, 1000);
        c.setDuration(qint64(200) * 3600000);
        QCOMPARE(c.durationMs, Countdowns::kLongestMs);
    }

    void timers_json()
    {
        Countdown a = fiveMinutes();
        a.name = "Tea";
        a.start(at(123));
        Countdown b = fiveMinutes();
        b.id = "u";
        b.pause(t0);
        const QList<Countdown> back = Countdowns::fromJson(Countdowns::toJson({a, b}));
        QCOMPARE(back.size(), 2);
        QCOMPARE(back[0], a);
        QCOMPARE(back[1], b);
        QCOMPARE(Countdowns::starters().size(), 4);
    }

    void stopwatch_laps()
    {
        Stopwatch s;
        s.start(t0);
        s.lap(at(10000));
        s.lap(at(18000));
        s.pause(at(20000));
        QCOMPARE(s.elapsedMs(at(99999)), 20000);
        s.start(at(30000));
        s.lap(at(42000));
        QCOMPARE(s.elapsedMs(at(42000)), 32000);
        const QList<Stopwatch::Row> rows = s.rows();
        QCOMPARE(rows.size(), 3);
        // newest first
        QCOMPARE(rows[0].number, 3);
        QCOMPARE(rows[0].lapMs, 14000);
        QCOMPARE(rows[0].totalMs, 32000);
        QVERIFY(rows[0].slowest);
        QVERIFY(rows[1].fastest);
        QVERIFY(!rows[2].fastest && !rows[2].slowest);
    }

    void stopwatch_two_laps_not_marked()
    {
        Stopwatch s;
        s.start(t0);
        s.lap(at(1000));
        s.lap(at(5000));
        for (const auto &r : s.rows())
            QVERIFY(!r.fastest && !r.slowest);
        s.pause(at(6000));
        s.lap(at(7000));
        QCOMPARE(s.laps.size(), 2);
    }

    void stopwatch_json()
    {
        Stopwatch s;
        s.start(t0);
        s.lap(at(1234));
        QCOMPARE(Stopwatch::fromJson(s.toJson()), s);
        s.reset();
        QCOMPARE(Stopwatch::fromJson(s.toJson()), s);
    }

    void texts()
    {
        QCOMPARE(Duration::clockText(300000), QStringLiteral("00:05:00"));
        // a timer shows 00:00:01 until it is over
        QCOMPARE(Duration::clockText(1), QStringLiteral("00:00:01"));
        QCOMPARE(Duration::clockText(0), QStringLiteral("00:00:00"));
        QCOMPARE(Duration::clockText(qint64(26) * 3600000 + 61000), QStringLiteral("26:01:01"));
        QCOMPARE(Duration::preciseText(65420), QStringLiteral("00:01:05.42"));
    }

    void until_rounds_up_to_the_minute()
    {
        Duration::Wait w = Duration::until(t0, at(30000));
        QCOMPARE(w.minutes, 1);
        w = Duration::until(t0, at((26 * 60 + 5) * 60000));
        QCOMPARE(w.days, 1);
        QCOMPARE(w.hours, 2);
        QCOMPARE(w.minutes, 5);
        w = Duration::until(t0, at(-5000));
        QCOMPARE(w.minutes, 0);
    }
};

QTEST_GUILESS_MAIN(TestCountdown)
#include "test_countdown.moc"
