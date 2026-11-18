// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Where the clock keeps things, and how they are written: each file whole,
// through a temporary file and a rename.
//
//   ~/.local/share/raye-clock/alarms.json      the alarms
//   ~/.local/share/raye-clock/timers.json      the timers
//   ~/.local/share/raye-clock/stopwatch.json   the stopwatch and its laps
//   ~/.local/state/raye-clock/rings.json       what rang, and the snoozes
//   ~/.config/raye/raye-clock.conf             the world clocks, window choices
//   ~/.config/systemd/user/raye-clock-ring.timer   the next ring

#pragma once

#include "rings.h"

#include <functional>

namespace ClockStore
{
QString alarmsPath();
QString timersPath();
QString stopwatchPath();
QString statePath();
QString settingsPath();
QString timerPath();

bool writeAtomic(const QString &path, const QByteArray &data, QString *error = nullptr);
// a missing file is empty, not an error
QByteArray read(const QString &path);
}

namespace Schedule
{
// runs `systemctl --user <args>`; tests give their own
using Systemctl = std::function<bool(const QStringList &args)>;
Systemctl realSystemctl();

enum class Action { StartNow, Timer, Nothing };

// Puts the timer where the rings need it: the service started right away
// when one is due, the timer at the next one otherwise, gone when none.
Action apply(const QList<Alarm> &alarms, const QList<Countdown> &timers, const RingState &state,
             const QDateTime &now, const QTimeZone &zone, const QString &timerPath, const Systemctl &systemctl);
}
