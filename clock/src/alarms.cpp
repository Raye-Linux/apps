// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "alarms.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

bool Alarms::ringsOn(int days, int dayOfWeek)
{
    return days == 0 || (dayOfWeek >= 1 && dayOfWeek <= 7 && (days & (1 << (dayOfWeek - 1))));
}

QDateTime Alarms::at(const QDate &day, const QTime &time, const QTimeZone &zone)
{
    // read with the offset before a change: a time met twice is the first one
    const QDateTime dt(day, time, zone, QDateTime::TransitionResolution::RelativeToBefore);
    if (dt.isValid() && dt.date() == day && dt.time() == time)
        return dt;
    // a skipped time comes out pushed past the gap: ring when the clock jumps
    const QTimeZone::OffsetData jump = zone.previousTransition(dt.addSecs(1));
    if (jump.atUtc.isValid() && jump.atUtc <= dt)
        return jump.atUtc.toTimeZone(zone);
    return dt;
}

QDateTime Alarms::nextAfter(const Alarm &alarm, const QDateTime &after, const QTimeZone &zone)
{
    if (!alarm.enabled || !alarm.time.isValid())
        return {};
    const QDate first = after.toTimeZone(zone).date();
    // a week and two days: a repeat day, and room for a change of hour
    for (int i = 0; i < 9; ++i) {
        const QDate day = first.addDays(i);
        if (!ringsOn(alarm.days, day.dayOfWeek()))
            continue;
        const QDateTime t = at(day, alarm.time, zone);
        if (t > after)
            return t;
    }
    return {};
}

QDateTime Alarms::lastBetween(const Alarm &alarm, const QDateTime &after, const QDateTime &upTo, const QTimeZone &zone)
{
    if (!alarm.enabled || !alarm.time.isValid() || upTo <= after)
        return {};
    if (!alarm.repeats()) {
        const QDateTime t = nextAfter(alarm, after, zone);
        return t.isValid() && t <= upTo ? t : QDateTime();
    }
    // nothing older than a week matters: it would only be "missed"
    QDateTime last;
    for (QDateTime t = nextAfter(alarm, qMax(after, upTo.addDays(-8)), zone); t.isValid() && t <= upTo;
         t = nextAfter(alarm, t, zone))
        last = t;
    return last;
}

QByteArray Alarms::toJson(const QList<Alarm> &alarms)
{
    QJsonArray list;
    for (const Alarm &a : alarms) {
        list.append(QJsonObject{
            {QStringLiteral("id"), a.id},
            {QStringLiteral("name"), a.name},
            {QStringLiteral("time"), a.time.toString(QStringLiteral("HH:mm"))},
            {QStringLiteral("days"), a.days},
            {QStringLiteral("enabled"), a.enabled},
            {QStringLiteral("sound"), a.sound},
            {QStringLiteral("snooze"), a.snoozeMinutes},
            {QStringLiteral("armed"), a.armed.toUTC().toString(Qt::ISODate)},
        });
    }
    return QJsonDocument(QJsonObject{{QStringLiteral("alarms"), list}}).toJson(QJsonDocument::Indented);
}

QList<Alarm> Alarms::fromJson(const QByteArray &data)
{
    QList<Alarm> out;
    const QJsonArray list = QJsonDocument::fromJson(data).object().value(QStringLiteral("alarms")).toArray();
    for (const QJsonValue &v : list) {
        const QJsonObject o = v.toObject();
        Alarm a;
        a.id = o.value(QStringLiteral("id")).toString();
        a.time = QTime::fromString(o.value(QStringLiteral("time")).toString(), QStringLiteral("HH:mm"));
        if (a.id.isEmpty() || !a.time.isValid())
            continue;
        a.name = o.value(QStringLiteral("name")).toString();
        a.days = o.value(QStringLiteral("days")).toInt() & kEveryDay;
        a.enabled = o.value(QStringLiteral("enabled")).toBool(true);
        a.sound = o.value(QStringLiteral("sound")).toString();
        a.snoozeMinutes = qBound(0, o.value(QStringLiteral("snooze")).toInt(10), 60);
        a.armed = QDateTime::fromString(o.value(QStringLiteral("armed")).toString(), Qt::ISODate);
        out << a;
    }
    return out;
}

QString Alarms::newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}
