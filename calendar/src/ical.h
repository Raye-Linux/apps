// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The calendar file: iCalendar (RFC 5545), the part of it a calendar of
// dates needs. VEVENT with its start and end (a day, a UTC time, a time
// in a named zone or a floating time), a simple RRULE, EXDATE and one
// VALARM. Lines this file does not use are kept and written back as
// they came, so an event made elsewhere loses nothing when it is saved.

#pragma once

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

struct Rule
{
    enum Freq { None, Daily, Weekly, Monthly, Yearly };
    Freq freq = None;
    int interval = 1;
    int count = 0;          // 0: no count
    QDateTime until;        // invalid: no end
    QList<int> byDay;       // 1 = Monday ... 7 = Sunday (weekly)
    int byDayNth = 0;       // monthly "2nd Tuesday": 2, last: -1, with one byDay
    int byMonthDay = 0;     // monthly "the 15th"
    QString raw;            // the RRULE as read, written back if untouched
    // parts this calendar cannot repeat by (BYSETPOS, several days of a
    // month...): only the first date is shown, the rule is kept as it was
    bool unsupported = false;

    bool isNull() const { return freq == None; }
    bool operator==(const Rule &) const = default;
};

struct CalEvent
{
    QString uid;
    QString summary;
    QString location;
    QString description;
    bool allDay = false;
    // timed: start and end; all-day: the days, end exclusive (iCalendar's way)
    QDateTime start;
    QDateTime end;
    QDate startDay;
    QDate endDay;
    Rule rule;
    QList<QDateTime> exdates;   // timed events
    QList<QDate> exdays;        // all-day events
    bool hasAlarm = false;
    int alarmMinutes = 0;       // minutes before the start; negative: after
    QString color;              // "#rrggbb", or empty for the accent
    QDateTime stamp;
    QStringList extra;          // VEVENT lines kept as they were read

    // how long one occurrence lasts, in seconds (all-day: whole days)
    qint64 lengthSecs() const;
};

// a whole file: the events, and the other components (VTODO, VTIMEZONE...)
// as raw lines, written back unchanged
struct CalFile
{
    QList<CalEvent> events;
    QStringList other;
};

namespace Ical
{
// Events without a start are dropped; anything not understood in an
// event is kept in `extra`.
CalFile parse(const QByteArray &data);
QByteArray serialize(const CalFile &file);

// RRULE text <-> Rule (the parts this file expands)
Rule parseRule(const QString &text);
QString ruleText(const Rule &rule);

// "20261111T143000Z", "20261111T143000" (floating) or with a TZID
QDateTime parseDateTime(const QString &value, const QString &tzid);
QString newUid();
}
