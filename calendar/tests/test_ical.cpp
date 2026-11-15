// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ical.h"

#include <QTest>
#include <QTimeZone>

class TestIcal : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("TZ", "America/Toronto");
    }

    void allDayEvent()
    {
        const auto f = Ical::parse("BEGIN:VCALENDAR\r\nBEGIN:VEVENT\r\nUID:a\r\nSUMMARY:Mom's birthday\r\n"
                                   "DTSTART;VALUE=DATE:20261203\r\nDTEND;VALUE=DATE:20261204\r\n"
                                   "RRULE:FREQ=YEARLY\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n");
        QCOMPARE(f.events.size(), 1);
        const CalEvent &e = f.events.first();
        QVERIFY(e.allDay);
        QCOMPARE(e.startDay, QDate(2026, 12, 3));
        QCOMPARE(e.endDay, QDate(2026, 12, 4));
        QCOMPARE(e.rule.freq, Rule::Yearly);
        QCOMPARE(e.summary, QStringLiteral("Mom's birthday"));
    }

    void allDayWithoutEnd()
    {
        const auto f = Ical::parse("BEGIN:VEVENT\nUID:b\nDTSTART;VALUE=DATE:20260101\nEND:VEVENT\n");
        QCOMPARE(f.events.size(), 1);
        QCOMPARE(f.events.first().endDay, QDate(2026, 1, 2));
    }

    void zones()
    {
        const auto f = Ical::parse("BEGIN:VEVENT\nUID:u\nDTSTART:20261111T143000Z\nDURATION:PT1H30M\nEND:VEVENT\n"
                                   "BEGIN:VEVENT\nUID:z\nDTSTART;TZID=Europe/Paris:20261111T090000\n"
                                   "DTEND;TZID=Europe/Paris:20261111T100000\nEND:VEVENT\n"
                                   "BEGIN:VEVENT\nUID:w\nDTSTART;TZID=Romance Standard Time:20260701T090000\nEND:VEVENT\n"
                                   "BEGIN:VEVENT\nUID:f\nDTSTART:20261111T080000\nEND:VEVENT\n");
        QCOMPARE(f.events.size(), 4);
        const CalEvent &utc = f.events[0];
        QCOMPARE(utc.start, QDateTime(QDate(2026, 11, 11), QTime(14, 30), QTimeZone::UTC));
        QCOMPARE(utc.lengthSecs(), 5400);
        const CalEvent &paris = f.events[1];
        QCOMPARE(paris.start.timeZone().id(), QByteArray("Europe/Paris"));
        // 9:00 in Paris is 8:00 UTC in November
        QCOMPARE(paris.start.toUTC().time(), QTime(8, 0));
        // a zone named the Windows way
        QCOMPARE(f.events[2].start.toUTC().time(), QTime(7, 0));
        // floating: the local 8:00, Toronto here
        QCOMPARE(f.events[3].start.timeSpec(), Qt::LocalTime);
        QCOMPARE(f.events[3].start.toUTC().time(), QTime(13, 0));
    }

    void foldedAndEscaped()
    {
        const auto f = Ical::parse("BEGIN:VEVENT\r\nUID:x\r\nDTSTART:20261111T143000Z\r\n"
                                   "SUMMARY:Dentist\\, then lunch\; bring the\r\n  papers\r\n"
                                   "DESCRIPTION:Line one\\nLine two\r\nEND:VEVENT\r\n");
        QCOMPARE(f.events.first().summary, QStringLiteral("Dentist, then lunch; bring the papers"));
        QCOMPARE(f.events.first().description, QStringLiteral("Line one\nLine two"));
    }

    void rules()
    {
        Rule r = Ical::parseRule(QStringLiteral("FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE;UNTIL=20261231T235959Z"));
        QCOMPARE(r.freq, Rule::Weekly);
        QCOMPARE(r.interval, 2);
        QCOMPARE(r.byDay, (QList<int>{1, 3}));
        QCOMPARE(r.until.toUTC().date(), QDate(2026, 12, 31));
        r = Ical::parseRule(QStringLiteral("FREQ=MONTHLY;BYDAY=-1FR;COUNT=6"));
        QCOMPARE(r.byDayNth, -1);
        QCOMPARE(r.byDay, (QList<int>{5}));
        QCOMPARE(r.count, 6);
        // a rule made here is written plainly
        Rule mine;
        mine.freq = Rule::Monthly;
        mine.byMonthDay = 15;
        QCOMPARE(Ical::ruleText(mine), QStringLiteral("FREQ=MONTHLY;BYMONTHDAY=15"));
        // a rule read and left alone goes back as it came
        r = Ical::parseRule(QStringLiteral("FREQ=YEARLY;BYMONTH=6;WKST=SU"));
        QCOMPARE(Ical::ruleText(r), QStringLiteral("FREQ=YEARLY;BYMONTH=6;WKST=SU"));
    }

    void alarms()
    {
        const auto f = Ical::parse("BEGIN:VEVENT\nUID:a\nDTSTART:20261111T150000Z\n"
                                   "BEGIN:VALARM\nACTION:DISPLAY\nTRIGGER:-PT15M\nEND:VALARM\n"
                                   "BEGIN:VALARM\nACTION:AUDIO\nTRIGGER:-P1D\nEND:VALARM\nEND:VEVENT\n"
                                   "BEGIN:VEVENT\nUID:b\nDTSTART:20261111T150000Z\n"
                                   "BEGIN:VALARM\nTRIGGER;VALUE=DATE-TIME:20261111T140000Z\nEND:VALARM\nEND:VEVENT\n");
        QVERIFY(f.events[0].hasAlarm);
        QCOMPARE(f.events[0].alarmMinutes, 15);
        // the second alarm is kept as it was
        QVERIFY(f.events[0].extra.contains(QStringLiteral("TRIGGER:-P1D")));
        QCOMPARE(f.events[1].alarmMinutes, 60);
    }

    void roundTrip()
    {
        CalEvent e;
        e.uid = QStringLiteral("r@raye");
        e.summary = QStringLiteral("Réunion, budget; été");
        e.start = QDateTime(QDate(2026, 11, 20), QTime(9, 30), QTimeZone("America/Toronto"));
        e.end = e.start.addSecs(3600);
        e.rule.freq = Rule::Weekly;
        e.rule.byDay = {5};
        e.exdates << e.start.addDays(7);
        e.hasAlarm = true;
        e.alarmMinutes = 10;
        e.color = QStringLiteral("#2f9e6f");
        e.extra << QStringLiteral("X-OTHER-APP;P=1:keep me");
        CalFile in;
        in.events << e;
        in.other << QStringLiteral("BEGIN:VTODO") << QStringLiteral("UID:t") << QStringLiteral("END:VTODO");
        const QByteArray text = Ical::serialize(in);
        // RFC 5545: no line longer than 75 octets, CRLF ends
        for (const QByteArray &l : text.split('\n'))
            QVERIFY2(l.size() <= 76, l.constData());
        QVERIFY(text.contains("DTSTART;TZID=America/Toronto:20261120T093000\r\n"));
        const CalFile out = Ical::parse(text);
        QCOMPARE(out.events.size(), 1);
        const CalEvent &o = out.events.first();
        QCOMPARE(o.summary, e.summary);
        QCOMPARE(o.start, e.start);
        QCOMPARE(o.end, e.end);
        QCOMPARE(o.rule.freq, Rule::Weekly);
        QCOMPARE(o.exdates, e.exdates);
        QCOMPARE(o.alarmMinutes, 10);
        QCOMPARE(o.color, e.color);
        QCOMPARE(o.extra, e.extra);
        QCOMPARE(out.other, in.other);
    }

    void longLineFoldsOnCharacters()
    {
        CalEvent e;
        e.uid = QStringLiteral("l");
        e.allDay = true;
        e.startDay = QDate(2026, 1, 1);
        e.endDay = QDate(2026, 1, 2);
        e.summary = QString(60, QChar(0x00e9));   // 120 octets of "é"
        const QByteArray text = Ical::serialize(CalFile{{e}, {}});
        QCOMPARE(Ical::parse(text).events.first().summary, e.summary);
        QVERIFY(QString::fromUtf8(text).contains(QChar(0xfffd)) == false);
    }

    void garbageIsSkipped()
    {
        const auto f = Ical::parse("BEGIN:VEVENT\nUID:no-start\nSUMMARY:x\nEND:VEVENT\nnot a line\n");
        QCOMPARE(f.events.size(), 0);
    }
};

QTEST_GUILESS_MAIN(TestIcal)
#include "test_ical.moc"
