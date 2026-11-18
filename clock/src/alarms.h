// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// An alarm and the moments it rings. Times are wall-clock times of the
// computer's zone: an alarm at 7:00 rings at 7:00 in summer and in winter.
// A time the clock skips when it moves forward rings when the clock jumps;
// a time it passes twice when it moves back rings the first time.

#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QTimeZone>

struct Alarm
{
    QString id;
    QString name;
    QTime time;
    // bit 0 is Monday, bit 6 Sunday; none: the alarm rings once
    int days = 0;
    bool enabled = true;
    QString sound;
    // 0: no Snooze
    int snoozeMinutes = 10;
    // when it was last switched on or changed: it never rings for a time before that
    QDateTime armed;

    bool repeats() const { return days != 0; }
    bool operator==(const Alarm &) const = default;
};

namespace Alarms
{
constexpr int kEveryDay = 0x7f;
constexpr int kWeekdays = 0x1f;

// dayOfWeek as QDate gives it, 1 (Monday) to 7
bool ringsOn(int days, int dayOfWeek);

// `time` on `day` in `zone`, with the two rules above
QDateTime at(const QDate &day, const QTime &time, const QTimeZone &zone);

// the first ring strictly after `after`; invalid for an alarm switched off
QDateTime nextAfter(const Alarm &alarm, const QDateTime &after, const QTimeZone &zone);
// the last ring in (after, upTo]; invalid when there is none
QDateTime lastBetween(const Alarm &alarm, const QDateTime &after, const QDateTime &upTo, const QTimeZone &zone);

QByteArray toJson(const QList<Alarm> &alarms);
QList<Alarm> fromJson(const QByteArray &data);
QString newId();
}
