// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rings.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <algorithm>

namespace
{
QJsonObject toObject(const QHash<QString, QDateTime> &h)
{
    QJsonObject o;
    for (auto it = h.cbegin(); it != h.cend(); ++it)
        o.insert(it.key(), it.value().toUTC().toString(Qt::ISODate));
    return o;
}

QHash<QString, QDateTime> fromObject(const QJsonObject &o)
{
    QHash<QString, QDateTime> h;
    for (auto it = o.begin(); it != o.end(); ++it) {
        const QDateTime t = QDateTime::fromString(it.value().toString(), Qt::ISODate);
        if (t.isValid())
            h.insert(it.key(), t);
    }
    return h;
}

bool isLate(const QDateTime &at, const QDateTime &now)
{
    return at.secsTo(now) > Rings::kLateSecs;
}
} // namespace

QByteArray RingState::toJson() const
{
    return QJsonDocument(QJsonObject{
                             {QStringLiteral("handled"), toObject(handled)},
                             {QStringLiteral("snoozed"), toObject(snoozed)},
                         })
        .toJson(QJsonDocument::Compact);
}

RingState RingState::fromJson(const QByteArray &data)
{
    const QJsonObject o = QJsonDocument::fromJson(data).object();
    RingState s;
    s.handled = fromObject(o.value(QStringLiteral("handled")).toObject());
    s.snoozed = fromObject(o.value(QStringLiteral("snoozed")).toObject());
    return s;
}

void RingState::keepOnly(const QList<Alarm> &alarms)
{
    QSet<QString> ids;
    for (const Alarm &a : alarms)
        ids.insert(a.id);
    for (auto *h : {&handled, &snoozed}) {
        for (auto it = h->begin(); it != h->end();)
            it = ids.contains(it.key()) ? std::next(it) : h->erase(it);
    }
}

QDateTime Rings::base(const Alarm &alarm, const RingState &state)
{
    const QDateTime done = state.handled.value(alarm.id);
    if (!alarm.armed.isValid())
        return done;
    return done.isValid() ? qMax(done, alarm.armed) : alarm.armed;
}

QList<Ring> Rings::due(const QList<Alarm> &alarms, const QList<Countdown> &timers, const RingState &state,
                       const QDateTime &now, const QTimeZone &zone)
{
    QList<Ring> out;
    for (const Alarm &a : alarms) {
        if (!a.enabled)
            continue;
        const QDateTime snooze = state.snoozed.value(a.id);
        if (snooze.isValid() && snooze <= now)
            out << Ring{Ring::Snooze, a.id, snooze, isLate(snooze, now)};
        // never armed (an old file): count from a day back, not from the beginning of time
        const QDateTime from = base(a, state).isValid() ? base(a, state) : now.addDays(-1);
        const QDateTime last = Alarms::lastBetween(a, from, now, zone);
        if (last.isValid())
            out << Ring{Ring::Alarm, a.id, last, isLate(last, now)};
    }
    for (const Countdown &c : timers) {
        if (c.over(now))
            out << Ring{Ring::Timer, c.id, c.endsAt, isLate(c.endsAt, now)};
    }
    std::sort(out.begin(), out.end(), [](const Ring &x, const Ring &y) { return x.at < y.at; });
    return out;
}

QDateTime Rings::nextOf(const Alarm &alarm, const RingState &state, const QDateTime &now, const QTimeZone &zone)
{
    if (!alarm.enabled)
        return {};
    const QDateTime from = base(alarm, state);
    QDateTime best = Alarms::nextAfter(alarm, from.isValid() ? qMax(from, now) : now, zone);
    const QDateTime snooze = state.snoozed.value(alarm.id);
    if (snooze.isValid() && snooze > now && (!best.isValid() || snooze < best))
        best = snooze;
    return best;
}

QDateTime Rings::next(const QList<Alarm> &alarms, const QList<Countdown> &timers, const RingState &state,
                      const QDateTime &now, const QTimeZone &zone)
{
    QDateTime best;
    auto offer = [&best](const QDateTime &t) {
        if (t.isValid() && (!best.isValid() || t < best))
            best = t;
    };
    for (const Alarm &a : alarms)
        offer(nextOf(a, state, now, zone));
    for (const Countdown &c : timers) {
        if (c.running() && c.endsAt > now)
            offer(c.endsAt);
    }
    return best;
}

QByteArray Rings::timerUnit(const QDateTime &at)
{
    // UTC, so a change of hour moves nothing
    const QString when = at.toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) + QStringLiteral(" UTC");
    return QStringLiteral(
               "# Written by Raye Clock for its next alarm or timer; rewritten each time.\n"
               "[Unit]\n"
               "Description=Raye Clock, next alarm\n"
               "\n"
               "[Timer]\n"
               "OnCalendar=%1\n"
               "AccuracySec=1s\n"
               // an alarm that came due while the computer was off is shown at sign-in
               "Persistent=true\n"
               "Unit=raye-clock-ring.service\n"
               "\n"
               "[Install]\n"
               "WantedBy=timers.target\n")
        .arg(when)
        .toUtf8();
}
