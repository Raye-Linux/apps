// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clockstore.h"

#include <QFile>
#include <QFileInfo>
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

class TestRings : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void alarm_due_once_then_handled()
    {
        const QList<Alarm> alarms{daily("a", 7)};
        RingState state;
        QVERIFY(Rings::due(alarms, {}, state, local(18, 6, 59), toronto).isEmpty());
        const QList<Ring> due = Rings::due(alarms, {}, state, local(18, 7, 0, 1), toronto);
        QCOMPARE(due.size(), 1);
        QCOMPARE(due.first().kind, Ring::Alarm);
        QCOMPARE(due.first().at, local(18, 7));
        QVERIFY(!due.first().late);
        state.handled.insert("a", due.first().at);
        QVERIFY(Rings::due(alarms, {}, state, local(18, 7, 1), toronto).isEmpty());
        QCOMPARE(Rings::next(alarms, {}, state, local(18, 7, 1), toronto), local(19, 7));
    }

    void long_ago_is_late()
    {
        const QList<Alarm> alarms{daily("a", 7)};
        RingState state;
        state.handled.insert("a", local(15, 7));
        // the computer slept three days: one ring, the latest, and late
        const QList<Alarm> old{[] { Alarm a = daily("a", 7); a.armed = local(1, 0); return a; }()};
        QCOMPARE(Rings::due(old, {}, state, local(18, 9), toronto).size(), 1);
        const QList<Ring> due = Rings::due(alarms, {}, state, local(18, 9), toronto);
        QCOMPARE(due.size(), 1);
        QCOMPARE(due.first().at, local(18, 7));
        QVERIFY(due.first().late);
    }

    void armed_after_the_time_waits_for_tomorrow()
    {
        Alarm a = daily("a", 7);
        a.armed = local(18, 7, 30);
        QVERIFY(Rings::due({a}, {}, {}, local(18, 8), toronto).isEmpty());
        QCOMPARE(Rings::next({a}, {}, {}, local(18, 8), toronto), local(19, 7));
    }

    void snooze_comes_back()
    {
        const QList<Alarm> alarms{daily("a", 7)};
        RingState state;
        state.handled.insert("a", local(18, 7));
        state.snoozed.insert("a", local(18, 7, 10));
        QCOMPARE(Rings::next(alarms, {}, state, local(18, 7, 1), toronto), local(18, 7, 10));
        QCOMPARE(Rings::nextOf(alarms.first(), state, local(18, 7, 1), toronto), local(18, 7, 10));
        const QList<Ring> due = Rings::due(alarms, {}, state, local(18, 7, 10), toronto);
        QCOMPARE(due.size(), 1);
        QCOMPARE(due.first().kind, Ring::Snooze);
    }

    void switched_off_drops_snooze()
    {
        Alarm a = daily("a", 7);
        a.enabled = false;
        RingState state;
        state.snoozed.insert("a", local(18, 7, 10));
        QVERIFY(Rings::due({a}, {}, state, local(18, 8), toronto).isEmpty());
        QVERIFY(!Rings::next({a}, {}, state, local(18, 6), toronto).isValid());
    }

    void timers_ring_at_their_end()
    {
        Countdown c;
        c.id = "t";
        c.setDuration(60000);
        c.start(local(18, 10));
        QCOMPARE(Rings::next({}, {c}, {}, local(18, 10), toronto), local(18, 10, 1));
        const QList<Ring> due = Rings::due({}, {c}, {}, local(18, 10, 1), toronto);
        QCOMPARE(due.size(), 1);
        QCOMPARE(due.first().kind, Ring::Timer);
        c.pause(local(18, 10, 0, 30));
        QVERIFY(!Rings::next({}, {c}, {}, local(18, 10), toronto).isValid());
    }

    void next_is_the_earliest()
    {
        Countdown c;
        c.id = "t";
        c.setDuration(3600000);
        c.start(local(18, 6));
        const QList<Alarm> alarms{daily("a", 8), daily("b", 6, 30)};
        QCOMPARE(Rings::next(alarms, {c}, {}, local(18, 6), toronto), local(18, 6, 30));
    }

    void state_json_and_forgetting()
    {
        RingState s;
        s.handled.insert("a", local(18, 7).toUTC());
        s.snoozed.insert("gone", local(18, 7, 10).toUTC());
        RingState back = RingState::fromJson(s.toJson());
        QCOMPARE(back.handled, s.handled);
        QCOMPARE(back.snoozed, s.snoozed);
        back.keepOnly({daily("a", 7)});
        QVERIFY(back.snoozed.isEmpty());
        QCOMPARE(back.handled.size(), 1);
    }

    void timer_unit_in_utc()
    {
        const QByteArray unit = Rings::timerUnit(local(18, 7));
        QVERIFY(unit.contains("OnCalendar=2026-11-18 12:00:00 UTC"));
        QVERIFY(unit.contains("Persistent=true"));
        QVERIFY(unit.contains("Unit=raye-clock-ring.service"));
    }

    void schedule_writes_and_removes_the_timer()
    {
        QTemporaryDir dir;
        const QString timer = dir.path() + "/systemd/user/raye-clock-ring.timer";
        QStringList calls;
        auto fake = [&calls](const QStringList &args) {
            calls << args.join(' ');
            return true;
        };
        const QList<Alarm> alarms{daily("a", 7)};
        QCOMPARE(Schedule::apply(alarms, {}, {}, local(18, 6), toronto, timer, fake), Schedule::Action::Timer);
        QVERIFY(QFile::exists(timer));
        QVERIFY(QFileInfo(dir.path() + "/systemd/user/timers.target.wants/raye-clock-ring.timer").isSymLink());
        QVERIFY(calls.contains("restart raye-clock-ring.timer"));

        calls.clear();
        QCOMPARE(Schedule::apply(alarms, {}, {}, local(18, 7, 0, 5), toronto, timer, fake), Schedule::Action::StartNow);
        QCOMPARE(calls, QStringList{"start --no-block raye-clock-ring.service"});

        Alarm off = alarms.first();
        off.enabled = false;
        QCOMPARE(Schedule::apply({off}, {}, {}, local(18, 6), toronto, timer, fake), Schedule::Action::Nothing);
        QVERIFY(!QFile::exists(timer));
    }

    void atomic_write_is_private()
    {
        QTemporaryDir dir;
        const QString path = dir.path() + "/a/b/alarms.json";
        QVERIFY(ClockStore::writeAtomic(path, "{}"));
        QCOMPARE(ClockStore::read(path), QByteArray("{}"));
        QCOMPARE(QFileInfo(path).permissions() & (QFile::ReadOther | QFile::ReadGroup), QFile::Permissions());
        QVERIFY(ClockStore::read(dir.path() + "/none").isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestRings)
#include "test_rings.moc"
