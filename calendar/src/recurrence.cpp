// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "recurrence.h"

#include <QTimeZone>

#include <algorithm>

namespace
{
// a safety net against a rule that never ends: about 270 years of days
constexpr int kMaxSteps = 100000;

QDateTime at(const CalEvent &ev, const QDate &day)
{
    if (ev.allDay)
        return QDateTime(day, QTime(0, 0));
    const QDateTime &s = ev.start;
    if (s.timeSpec() == Qt::TimeZone)
        return QDateTime(day, s.time(), s.timeZone());
    if (s.timeSpec() == Qt::UTC)
        return QDateTime(day, s.time(), QTimeZone::UTC);
    return QDateTime(day, s.time());
}

QDate nthWeekday(int year, int month, int weekday, int nth)
{
    const QDate first(year, month, 1);
    if (nth > 0) {
        const int shift = (weekday - first.dayOfWeek() + 7) % 7;
        const QDate d = first.addDays(shift + 7 * (nth - 1));
        return d.month() == month ? d : QDate();
    }
    const QDate last(year, month, first.daysInMonth());
    const int shift = (last.dayOfWeek() - weekday + 7) % 7;
    const QDate d = last.addDays(-shift - 7 * (-nth - 1));
    return d.month() == month ? d : QDate();
}

// the days of one period of the rule, in order (may be empty)
QList<QDate> period(const CalEvent &ev, const QDate &first, int k)
{
    const Rule &r = ev.rule;
    switch (r.freq) {
    case Rule::Daily:
        return {first.addDays(qint64(k) * r.interval)};
    case Rule::Weekly: {
        const QDate monday = first.addDays(1 - first.dayOfWeek()).addDays(qint64(k) * 7 * r.interval);
        QList<int> days = r.byDay.isEmpty() ? QList<int>{first.dayOfWeek()} : r.byDay;
        std::sort(days.begin(), days.end());
        QList<QDate> out;
        for (int d : days)
            out << monday.addDays(d - 1);
        return out;
    }
    case Rule::Monthly: {
        const QDate m = QDate(first.year(), first.month(), 1).addMonths(k * r.interval);
        if (r.byDayNth && !r.byDay.isEmpty())
            return {nthWeekday(m.year(), m.month(), r.byDay.first(), r.byDayNth)};
        int day = r.byMonthDay ? r.byMonthDay : first.day();
        if (day < 0)
            day = m.daysInMonth() + 1 + day;
        // the 31st is skipped in shorter months, as RFC 5545 says
        return {QDate(m.year(), m.month(), day)};
    }
    case Rule::Yearly:
        // February 29th only in leap years
        return {QDate(first.year() + k * r.interval, first.month(), first.day())};
    case Rule::None:
        break;
    }
    return {};
}
} // namespace

QList<Occurrence> Recurrence::between(const CalEvent &ev, const QDateTime &from, const QDateTime &to)
{
    QList<Occurrence> out;
    const QDate firstDay = ev.allDay ? ev.startDay : ev.start.date();
    const qint64 length = ev.lengthSecs();
    const QDateTime firstStart = at(ev, firstDay);

    auto take = [&](const QDate &day) {
        if (ev.allDay ? ev.exdays.contains(day) : ev.exdates.contains(at(ev, day)))
            return;
        Occurrence o;
        o.day = day;
        o.start = at(ev, day);
        o.end = ev.allDay ? QDateTime(day.addDays(length / 86400), QTime(0, 0)) : o.start.addSecs(length);
        const bool overlaps = length > 0 ? (o.end > from && o.start < to) : (o.start >= from && o.start < to);
        if (overlaps)
            out << o;
    };

    // a rule read only in part would give dates that are not the event's
    if (ev.rule.isNull() || ev.rule.unsupported) {
        take(firstDay);
        return out;
    }

    int counted = 0;
    for (int k = 0; k < kMaxSteps; ++k) {
        bool past = false;
        for (const QDate &day : period(ev, firstDay, k)) {
            if (!day.isValid())
                continue;
            const QDateTime start = at(ev, day);
            if (start < firstStart)
                continue;
            if (ev.rule.until.isValid() && start > ev.rule.until)
                return out;
            if (ev.rule.count && counted >= ev.rule.count)
                return out;
            ++counted;
            if (start >= to) {
                past = true;
                break;
            }
            take(day);
        }
        if (past)
            break;
    }
    return out;
}
