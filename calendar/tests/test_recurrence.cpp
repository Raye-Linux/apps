// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "recurrence.h"

#include <QTest>
#include <QTimeZone>

namespace
{
CalEvent timed(const char *rule, QDateTime start)
{
    CalEvent e;
    e.uid = QStringLiteral("t");
    e.start = start;
    e.end = start.addSecs(3600);
    if (rule)
        e.rule = Ical::parseRule(QString::fromLatin1(rule));
    return e;
}

QList<QDate> days(const QList<Occurrence> &occ)
{
    QList<QDate> d;
    for (const auto &o : occ)
        d << o.day;
    return d;
}

QDateTime local(int y, int m, int d, int h = 0)
{
    return QDateTime(QDate(y, m, d), QTime(h, 0));
}
} // namespace

class TestRecurrence : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("TZ", "America/Toronto");
    }

    void single()
    {
        const CalEvent e = timed(nullptr, local(2026, 11, 11, 14));
        QCOMPARE(Recurrence::between(e, local(2026, 11, 11), local(2026, 11, 12)).size(), 1);
        QCOMPARE(Recurrence::between(e, local(2026, 11, 12), local(2026, 11, 13)).size(), 0);
        // an event under way when the range starts is in it
        QCOMPARE(Recurrence::between(e, local(2026, 11, 11, 14).addSecs(1800), local(2026, 11, 12)).size(), 1);
    }

    // two days a month: shown once, not as "the first Monday" only
    void unsupportedRuleShowsTheFirstDateOnly()
    {
        const CalEvent e = timed("FREQ=MONTHLY;BYDAY=1MO,1FR", local(2026, 11, 2, 9));
        QVERIFY(e.rule.unsupported);
        QCOMPARE(days(Recurrence::between(e, local(2026, 10, 1), local(2027, 3, 1))), (QList<QDate>{{2026, 11, 2}}));
        QVERIFY(Ical::parseRule(QStringLiteral("FREQ=MONTHLY;BYSETPOS=-1;BYDAY=MO,TU")).unsupported);
        QVERIFY(Ical::parseRule(QStringLiteral("FREQ=YEARLY;BYMONTH=3;BYDAY=-1SU")).unsupported);
        QVERIFY(Ical::parseRule(QStringLiteral("FREQ=DAILY;BYHOUR=9,17")).unsupported);
        QVERIFY(!Ical::parseRule(QStringLiteral("FREQ=MONTHLY;BYDAY=-1FR")).unsupported);
        QVERIFY(!Ical::parseRule(QStringLiteral("FREQ=WEEKLY;BYDAY=MO,WE;WKST=MO")).unsupported);
        QVERIFY(!Ical::parseRule(QStringLiteral("FREQ=MONTHLY;BYMONTHDAY=15")).unsupported);
    }

    void dailyWithCount()
    {
        const CalEvent e = timed("FREQ=DAILY;COUNT=3", local(2026, 11, 1, 9));
        QCOMPARE(days(Recurrence::between(e, local(2026, 10, 1), local(2026, 12, 1))),
                 (QList<QDate>{{2026, 11, 1}, {2026, 11, 2}, {2026, 11, 3}}));
    }

    void weeklyDays()
    {
        // Monday the 2nd: every other week, Monday and Friday
        const CalEvent e = timed("FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,FR", local(2026, 11, 2, 9));
        QCOMPARE(days(Recurrence::between(e, local(2026, 11, 1), local(2026, 11, 30))),
                 (QList<QDate>{{2026, 11, 2}, {2026, 11, 6}, {2026, 11, 16}, {2026, 11, 20}}));
    }

    void untilIsKept()
    {
        const CalEvent e = timed("FREQ=WEEKLY;UNTIL=20261116T140000Z", local(2026, 11, 2, 9));
        QCOMPARE(days(Recurrence::between(e, local(2026, 11, 1), local(2027, 1, 1))),
                 (QList<QDate>{{2026, 11, 2}, {2026, 11, 9}, {2026, 11, 16}}));
    }

    void monthlySkipsShortMonths()
    {
        const CalEvent e = timed("FREQ=MONTHLY", local(2026, 1, 31, 9));
        QCOMPARE(days(Recurrence::between(e, local(2026, 1, 1), local(2026, 6, 1))),
                 (QList<QDate>{{2026, 1, 31}, {2026, 3, 31}, {2026, 5, 31}}));
    }

    void monthlyLastFriday()
    {
        const CalEvent e = timed("FREQ=MONTHLY;BYDAY=-1FR", local(2026, 11, 27, 16));
        QCOMPARE(days(Recurrence::between(e, local(2026, 11, 1), local(2027, 2, 1))),
                 (QList<QDate>{{2026, 11, 27}, {2026, 12, 25}, {2027, 1, 29}}));
    }

    void yearlyLeapDay()
    {
        CalEvent e;
        e.uid = QStringLiteral("b");
        e.allDay = true;
        e.startDay = QDate(2028, 2, 29);
        e.endDay = QDate(2028, 3, 1);
        e.rule.freq = Rule::Yearly;
        QCOMPARE(days(Recurrence::between(e, local(2028, 1, 1), local(2036, 12, 31))),
                 (QList<QDate>{{2028, 2, 29}, {2032, 2, 29}, {2036, 2, 29}}));
    }

    void exdate()
    {
        CalEvent e = timed("FREQ=DAILY;COUNT=3", local(2026, 11, 1, 9));
        e.exdates << local(2026, 11, 2, 9);
        QCOMPARE(days(Recurrence::between(e, local(2026, 10, 1), local(2026, 12, 1))),
                 (QList<QDate>{{2026, 11, 1}, {2026, 11, 3}}));
    }

    void sameHourAcrossTheChangeOfHour()
    {
        // Toronto leaves summer time on 2026-11-01: 9:00 stays 9:00
        const QTimeZone zone("America/Toronto");
        const CalEvent e = timed("FREQ=WEEKLY", QDateTime(QDate(2026, 10, 26), QTime(9, 0), zone));
        const auto occ = Recurrence::between(e, local(2026, 10, 25), local(2026, 11, 8));
        QCOMPARE(occ.size(), 2);
        QCOMPARE(occ[0].start.toUTC().time(), QTime(13, 0));
        QCOMPARE(occ[1].start.toUTC().time(), QTime(14, 0));
        QCOMPARE(occ[1].start.time(), QTime(9, 0));
    }

    void allDayRangeAndLength()
    {
        CalEvent e;
        e.uid = QStringLiteral("v");
        e.allDay = true;
        e.startDay = QDate(2026, 12, 24);
        e.endDay = QDate(2026, 12, 27);
        const auto occ = Recurrence::between(e, local(2026, 12, 26), local(2026, 12, 27));
        QCOMPARE(occ.size(), 1);
        QCOMPARE(occ.first().end, local(2026, 12, 27));
    }
};

QTEST_GUILESS_MAIN(TestRecurrence)
#include "test_recurrence.moc"
