// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Waking the PC from sleep for the next alarm. A user timer cannot do it
// (WakeSystem= needs a system timer), so raye-clock-wake, run as root
// through pkexec, keeps one system timer per user in /run/systemd/system:
// raye-clock-wake-<uid>.timer, WakeSystem=true, a little before the alarm.
// The user timer then rings it as usual once the PC is up. Only alarms and
// snoozes wake the PC, never a timer. A PC that is shut down stays off.

#pragma once

#include "rings.h"

#include <functional>

namespace Wake
{
// the PC wakes this long before the alarm, so it rings on time
constexpr qint64 kLeadSecs = 30;

// the next alarm or snooze; invalid when none is on
QDateTime nextAlarm(const QList<Alarm> &alarms, const RingState &state, const QDateTime &now, const QTimeZone &zone);
// when to wake for an alarm at `at`: kLeadSecs before it, never in the past
qint64 wakeEpoch(const QDateTime &at, const QDateTime &now);

// the system timer the helper writes, and its name for a user
QByteArray timerUnit(qint64 epoch);
QString unitName(uint uid);
// what the helper accepts: from a minute ago to a year and a bit ahead
bool validEpoch(qint64 epoch, qint64 now);
// PKEXEC_UID, digits only
uint parseUid(const QByteArray &text, bool *ok);

// the PC has a clock that can wake it (sysRoot is /sys but in tests)
bool rtcCanWake(const QString &sysRoot = QStringLiteral("/sys"));

// runs `pkexec raye-clock-wake <args>`; tests give their own
using Helper = std::function<bool(const QStringList &args)>;
Helper realHelper();
// the helper is installed (pkexec and raye-clock-wake)
bool helperInstalled();

enum class Result { Set, Cleared, Unchanged, Unavailable, Failed };
// Sets the wake for `nextAlarm` (or clears it when invalid). `lastPath`
// keeps what was set, so the helper runs only when that changes.
Result apply(const QDateTime &nextAlarm, const QDateTime &now, const QString &lastPath, bool canWake, const Helper &helper);
}
