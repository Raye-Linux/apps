// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Which alarms and timers ring now, and when the next one does. Nothing
// runs in between: raye-clock-tool writes one systemd user timer for the
// next ring and is started by it, so an alarm rings with the window closed.
//
// A ring that came due long ago (the computer was asleep or off) is
// "late": it is shown as missed, without a sound.

#pragma once

#include "alarms.h"
#include "countdown.h"

#include <QHash>

struct RingState
{
    // alarm id -> the last time of it that was rung (or shown as missed)
    QHash<QString, QDateTime> handled;
    // alarm id -> when its snooze ends
    QHash<QString, QDateTime> snoozed;

    QByteArray toJson() const;
    static RingState fromJson(const QByteArray &data);
    // forgets the alarms that are gone
    void keepOnly(const QList<Alarm> &alarms);
};

struct Ring
{
    enum Kind { Alarm, Snooze, Timer };
    Kind kind = Alarm;
    QString id;
    QDateTime at;
    bool late = false;
};

namespace Rings
{
// later than this, a ring is shown as missed and makes no sound
constexpr qint64 kLateSecs = 15 * 60;

// the moment an alarm can next ring after: when it was switched on, or
// the time it last rang, whichever is later
QDateTime base(const Alarm &alarm, const RingState &state);

QList<Ring> due(const QList<Alarm> &alarms, const QList<Countdown> &timers, const RingState &state,
                const QDateTime &now, const QTimeZone &zone);
// the first moment after `now` something rings; invalid when nothing will
QDateTime next(const QList<Alarm> &alarms, const QList<Countdown> &timers, const RingState &state,
               const QDateTime &now, const QTimeZone &zone);
// the next ring of one alarm, its snooze included; invalid when off
QDateTime nextOf(const Alarm &alarm, const RingState &state, const QDateTime &now, const QTimeZone &zone);

// the timer unit that starts raye-clock-ring.service at `at`
QByteArray timerUnit(const QDateTime &at);
}
