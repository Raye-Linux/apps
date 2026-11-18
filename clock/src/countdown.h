// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Timers and the stopwatch. Both keep a moment, not a running count, so
// they go on while the window is closed and come back right when it opens:
// a running timer is the time it ends, a running stopwatch the time it
// started plus what it had before.

#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

struct Countdown
{
    QString id;
    QString name;
    qint64 durationMs = 0;
    // what is left while it stands still
    qint64 leftMs = 0;
    // when it ends, while it runs
    QDateTime endsAt;

    bool running() const { return endsAt.isValid(); }
    qint64 remainingMs(const QDateTime &now) const;
    // 1 when full, 0 when over
    double fraction(const QDateTime &now) const;
    bool over(const QDateTime &now) const { return running() && endsAt <= now; }

    void start(const QDateTime &now);
    void pause(const QDateTime &now);
    // back to its whole length, standing still
    void reset();
    void restart(const QDateTime &now);
    // a new length resets it
    void setDuration(qint64 ms);

    bool operator==(const Countdown &) const = default;
};

namespace Countdowns
{
constexpr qint64 kLongestMs = qint64(99) * 3600 * 1000 + 59 * 60 * 1000 + 59 * 1000;

QByteArray toJson(const QList<Countdown> &timers);
QList<Countdown> fromJson(const QByteArray &data);
// the timers a new user starts with, as on Windows: 1, 3, 5 and 10 minutes
QList<Countdown> starters();
}

struct Stopwatch
{
    qint64 bankedMs = 0;
    // while it runs
    QDateTime since;
    // the total at each lap, oldest first
    QList<qint64> laps;

    bool running() const { return since.isValid(); }
    qint64 elapsedMs(const QDateTime &now) const;
    void start(const QDateTime &now);
    void pause(const QDateTime &now);
    void lap(const QDateTime &now);
    void reset();

    struct Row
    {
        int number = 0;
        qint64 lapMs = 0;
        qint64 totalMs = 0;
        bool fastest = false;
        bool slowest = false;
    };
    // newest first; fastest and slowest are marked once there are three laps
    QList<Row> rows() const;

    QByteArray toJson() const;
    static Stopwatch fromJson(const QByteArray &data);
    bool operator==(const Stopwatch &) const = default;
};

namespace Duration
{
struct Parts
{
    qint64 hours = 0;
    int minutes = 0;
    int seconds = 0;
    int hundredths = 0;
};
Parts split(qint64 ms);
// "01:05:09" for a timer: seconds rounded up, so it shows 00:00:01 until the end
QString clockText(qint64 ms);
// "00:01:05.42" for the stopwatch and its laps
QString preciseText(qint64 ms);
struct Wait
{
    int days = 0;
    int hours = 0;
    int minutes = 0;
};
// what is left until `at`, in whole minutes rounded up
Wait until(const QDateTime &now, const QDateTime &at);
}
