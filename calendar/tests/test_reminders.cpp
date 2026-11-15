// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "reminders.h"

#include <QTest>
#include <QTimeZone>

namespace
{
QDateTime utc(int d, int h, int m = 0)
{
    return QDateTime(QDate(2026, 11, d), QTime(h, m), QTimeZone::UTC);
}

CalEvent meeting(const QDateTime &start, int minutesBefore, const char *rule = nullptr)
{
    CalEvent e;
    e.uid = QStringLiteral("m");
    e.summary = QStringLiteral("Meeting");
    e.start = start;
    e.end = start.addSecs(1800);
    e.hasAlarm = true;
    e.alarmMinutes = minutesBefore;
    if (rule)
        e.rule = Ical::parseRule(QString::fromLatin1(rule));
    return e;
}
} // namespace

class TestReminders : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("TZ", "America/Toronto");
    }

    void nextIsTheTriggerTime()
    {
        const QList<CalEvent> ev{meeting(utc(11, 15), 15)};
        QCOMPARE(Reminders::next(ev, {}, utc(11, 12)), utc(11, 14, 45));
        QVERIFY(Reminders::due(ev, {}, utc(11, 14, 44)).isEmpty());
        QCOMPARE(Reminders::due(ev, {}, utc(11, 14, 45)).size(), 1);
    }

    void noAlarmNoTimer()
    {
        CalEvent e = meeting(utc(11, 15), 15);
        e.hasAlarm = false;
        QVERIFY(!Reminders::next({e}, {}, utc(11, 12)).isValid());
    }

    void doneIsNotShownTwice()
    {
        const QList<CalEvent> ev{meeting(utc(11, 15), 15)};
        const auto due = Reminders::due(ev, {}, utc(11, 14, 50));
        QCOMPARE(due.size(), 1);
        ReminderState s;
        s.done.insert(due.first().key, utc(11, 14, 50));
        QVERIFY(Reminders::due(ev, s, utc(11, 14, 51)).isEmpty());
        QVERIFY(!Reminders::next(ev, s, utc(11, 14, 51)).isValid());
    }

    void recurringGoesToTheNextOne()
    {
        const QList<CalEvent> ev{meeting(utc(9, 15), 10, "FREQ=DAILY")};
        ReminderState s;
        s.done.insert(Reminders::due(ev, {}, utc(11, 14, 50)).first().key, utc(11, 14, 50));
        // today's is done: tomorrow's
        QCOMPARE(Reminders::next(ev, s, utc(11, 14, 51)), utc(12, 14, 50));
    }

    void snooze()
    {
        const QList<CalEvent> ev{meeting(utc(11, 15), 15)};
        const QString k = Reminders::due(ev, {}, utc(11, 14, 45)).first().key;
        ReminderState s;
        s.done.insert(k, utc(11, 14, 45));
        s.snoozed.insert(k, utc(11, 14, 50));
        QCOMPARE(Reminders::next(ev, s, utc(11, 14, 46)), utc(11, 14, 50));
        QVERIFY(Reminders::due(ev, s, utc(11, 14, 49)).isEmpty());
        QCOMPARE(Reminders::due(ev, s, utc(11, 14, 50)).size(), 1);
    }

    void missedWhileOffStillShowsIfNotOver()
    {
        // the computer was off at 14:45; signed in at 15:10, the meeting runs to 15:30
        const QList<CalEvent> ev{meeting(utc(11, 15), 15)};
        QCOMPARE(Reminders::due(ev, {}, utc(11, 15, 10)).size(), 1);
        // the next morning it is over and nobody wants it any more
        QVERIFY(Reminders::due(ev, {}, utc(12, 9)).isEmpty());
    }

    void addedLateShowsAtOnce()
    {
        // made at 14:55 for 15:00 with "15 minutes before": the time is gone, the meeting is not
        const QList<CalEvent> ev{meeting(utc(11, 15), 15)};
        QCOMPARE(Reminders::due(ev, {}, utc(11, 14, 55)).size(), 1);
    }

    void allDayAtNineTheDayBefore()
    {
        CalEvent e;
        e.uid = QStringLiteral("b");
        e.allDay = true;
        e.startDay = QDate(2026, 12, 3);
        e.endDay = QDate(2026, 12, 4);
        e.hasAlarm = true;
        e.alarmMinutes = 15 * 60;   // midnight minus 15 hours: 9:00 the day before
        QCOMPARE(Reminders::next({e}, {}, utc(11, 12)), QDateTime(QDate(2026, 12, 2), QTime(9, 0)));
    }

    // more than the horizon away: the timer still wakes in time to look again
    void farAwayIsNotForgotten()
    {
        const QDateTime now = utc(11, 12);
        const CalEvent far = meeting(now.addDays(800), 15);
        const QDateTime next = Reminders::next({far}, {}, now);
        QVERIFY(next.isValid());
        QVERIFY(next > now && next <= now.addDays(400));
    }

    // a notification left open keeps the tool running: it must still wake
    void wakeWhileShown()
    {
        const QDateTime now = utc(11, 12);
        QCOMPARE(Reminders::wakeWhileShownMs(now, now.addSecs(600)), 600000);
        QCOMPARE(Reminders::wakeWhileShownMs(now, now.addSecs(13 * 3600)), 6 * 3600 * 1000);
        QCOMPARE(Reminders::wakeWhileShownMs(now, QDateTime()), -1);
    }

    void pruneForgetsOldOnes()
    {
        ReminderState s;
        s.done.insert(QStringLiteral("old"), utc(1, 9));
        s.done.insert(QStringLiteral("new"), utc(11, 9));
        s.prune(utc(11, 12));
        QCOMPARE(s.done.keys(), QStringList{QStringLiteral("new")});
        const ReminderState back = ReminderState::fromJson(s.toJson());
        QCOMPARE(back.done.value(QStringLiteral("new")), utc(11, 9));
    }

    void timerIsInUtc()
    {
        const QByteArray unit = Reminders::timerUnit(QDateTime(QDate(2026, 11, 11), QTime(9, 45), QTimeZone("America/Toronto")));
        QVERIFY(unit.contains("OnCalendar=2026-11-11 14:45:00 UTC\n"));
        QVERIFY(unit.contains("Persistent=true\n"));
        QVERIFY(unit.contains("Unit=raye-calendar-reminder.service\n"));
    }
};

QTEST_GUILESS_MAIN(TestReminders)
#include "test_reminders.moc"
