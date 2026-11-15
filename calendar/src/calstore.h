// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Where the calendar lives and how it is written. One file, written whole
// through a temporary file and a rename, so a crash or a full disk leaves
// the old calendar, never half of a new one.
//
//   ~/.local/share/raye-calendar/calendar.ics     the events
//   ~/.local/state/raye-calendar/reminders.json   which reminders were shown
//   ~/.config/raye/raye-calendar.conf             the window's choices
//   ~/.config/systemd/user/raye-calendar-reminder.timer   the next reminder

#pragma once

#include "ical.h"
#include "reminders.h"

#include <functional>

namespace CalStore
{
QString calendarPath();
QString statePath();
QString settingsPath();
QString timerPath();

bool load(const QString &path, CalFile *out, QString *error = nullptr);
bool save(const QString &path, const CalFile &file, QString *error = nullptr);
// a missing file is an empty calendar, not an error
CalFile loadOrEmpty(const QString &path);

ReminderState loadState(const QString &path);
bool saveState(const QString &path, const ReminderState &state);

bool writeAtomic(const QString &path, const QByteArray &data, QString *error = nullptr);

// Adds the events of an imported file: the same UID replaces, a new one is
// added. A changed single occurrence (RECURRENCE-ID) is left out, since it
// would replace its whole series. Returns how many came in.
int mergeImport(CalFile &into, const CalFile &in, int *leftOut = nullptr);
}

namespace Schedule
{
// runs `systemctl --user <args>`; tests give their own
using Systemctl = std::function<bool(const QStringList &args)>;
Systemctl realSystemctl();

enum class Action { StartNow, Timer, Nothing };

// Puts the timer where the reminders need it: started right away when one
// is due, at the next one otherwise, gone when there is none.
Action apply(const QList<CalEvent> &events, const ReminderState &state, const QDateTime &now,
             const QString &timerPath, const Systemctl &systemctl);
}
