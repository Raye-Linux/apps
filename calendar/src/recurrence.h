// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// When an event happens: its start, and every repetition of its RRULE,
// minus its EXDATEs. A timed event repeats at the same time of day in its
// own zone, so a 9:00 meeting stays at 9:00 across a change of hour.

#pragma once

#include "ical.h"

struct Occurrence
{
    QDateTime start;   // all-day: 00:00 of the day, local time
    QDateTime end;
    QDate day;
};

namespace Recurrence
{
// occurrences that overlap [from, to), in order
QList<Occurrence> between(const CalEvent &ev, const QDateTime &from, const QDateTime &to);
}
