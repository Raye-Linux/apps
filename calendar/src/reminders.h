// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Which reminders are due, and when the next one is. Nothing runs between
// two reminders: raye-calendar-tool writes one systemd user timer for the
// next one, and is started by it.
//
// A reminder is the pair (event, occurrence). Once shown it is "done";
// Snooze puts it back for a few minutes. A reminder whose time went by
// while the computer was off is still shown if its event is not over.

#pragma once

#include "recurrence.h"

#include <QHash>

struct Reminder
{
    int event = -1;          // index in the list given
    Occurrence occurrence;
    QDateTime at;            // when it is due
    QString key;
};

struct ReminderState
{
    QHash<QString, QDateTime> done;     // key -> when it was shown
    QHash<QString, QDateTime> snoozed;  // key -> until

    QByteArray toJson() const;
    static ReminderState fromJson(const QByteArray &data);
    // forget what is older than two days: those can never come back
    void prune(const QDateTime &now);
};

namespace Reminders
{
QString key(const CalEvent &ev, const Occurrence &o);
QDateTime triggerOf(const CalEvent &ev, const Occurrence &o);

QList<Reminder> due(const QList<CalEvent> &events, const ReminderState &state, const QDateTime &now);
// how long a tool still showing reminders sleeps before it looks again:
// never more than a few hours, since systemd cannot start it while it runs.
// -1 when there is nothing to wait for
qint64 wakeWhileShownMs(const QDateTime &now, const QDateTime &next);
// the first moment after `now` a reminder is due; invalid when there is none,
// or a day short of a year away when one is further than that
QDateTime next(const QList<CalEvent> &events, const ReminderState &state, const QDateTime &now);

// the timer unit that starts raye-calendar-reminder.service at `at`
QByteArray timerUnit(const QDateTime &at);
}
