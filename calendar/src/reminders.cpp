// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "reminders.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace
{
// how far ahead next() looks: a yearly birthday, plus the longest lead
constexpr qint64 kHorizonDays = 400;
// a reminder whose event has no length stays worth showing this long
constexpr qint64 kGraceSecs = 3600;

QDateTime relevantUntil(const Occurrence &o)
{
    return qMax(o.end, o.start.addSecs(kGraceSecs));
}

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
} // namespace

QByteArray ReminderState::toJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("done"), toObject(done));
    o.insert(QStringLiteral("snoozed"), toObject(snoozed));
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

ReminderState ReminderState::fromJson(const QByteArray &data)
{
    ReminderState s;
    const QJsonObject o = QJsonDocument::fromJson(data).object();
    s.done = fromObject(o.value(QStringLiteral("done")).toObject());
    s.snoozed = fromObject(o.value(QStringLiteral("snoozed")).toObject());
    return s;
}

void ReminderState::prune(const QDateTime &now)
{
    const QDateTime limit = now.addDays(-2);
    for (auto it = done.begin(); it != done.end();)
        it = it.value() < limit ? done.erase(it) : std::next(it);
}

QString Reminders::key(const CalEvent &ev, const Occurrence &o)
{
    return ev.uid + QLatin1Char('|') + o.start.toUTC().toString(Qt::ISODate);
}

QDateTime Reminders::triggerOf(const CalEvent &ev, const Occurrence &o)
{
    return o.start.addSecs(-qint64(ev.alarmMinutes) * 60);
}

QList<Reminder> Reminders::due(const QList<CalEvent> &events, const ReminderState &state, const QDateTime &now)
{
    QList<Reminder> out;
    for (int i = 0; i < events.size(); ++i) {
        const CalEvent &ev = events[i];
        if (!ev.hasAlarm)
            continue;
        // the occurrences whose reminder may be due by now
        const QDateTime from = now.addDays(-2);
        const QDateTime to = now.addSecs(qMax<qint64>(0, qint64(ev.alarmMinutes) * 60) + 1);
        for (const Occurrence &o : Recurrence::between(ev, from, to)) {
            const QString k = key(ev, o);
            const QDateTime trigger = triggerOf(ev, o);
            const auto snooze = state.snoozed.constFind(k);
            if (snooze != state.snoozed.cend()) {
                if (snooze.value() <= now)
                    out << Reminder{i, o, snooze.value(), k};
                continue;
            }
            if (trigger > now || state.done.contains(k) || relevantUntil(o) <= now)
                continue;
            out << Reminder{i, o, trigger, k};
        }
    }
    std::sort(out.begin(), out.end(), [](const Reminder &a, const Reminder &b) { return a.at < b.at; });
    return out;
}

QDateTime Reminders::next(const QList<CalEvent> &events, const ReminderState &state, const QDateTime &now)
{
    QDateTime best;
    auto offer = [&best](const QDateTime &t) {
        if (!best.isValid() || t < best)
            best = t;
    };
    for (const QDateTime &until : state.snoozed) {
        if (until > now)
            offer(until);
    }
    bool lookAgain = false;
    for (const CalEvent &ev : events) {
        if (!ev.hasAlarm)
            continue;
        // the start that puts the reminder just after now
        const QDateTime from = now.addSecs(qint64(ev.alarmMinutes) * 60);
        const QDateTime horizon = from.addDays(kHorizonDays);
        // beyond the horizon: nothing would ever set the timer for it
        const QDateTime first = ev.allDay ? QDateTime(ev.startDay, QTime(0, 0)) : ev.start;
        if (ev.rule.isNull() ? first > horizon : ev.rule.count > 0 || !ev.rule.until.isValid() || ev.rule.until > horizon)
            lookAgain = true;
        for (const Occurrence &o : Recurrence::between(ev, from, horizon)) {
            const QDateTime trigger = triggerOf(ev, o);
            const QString k = key(ev, o);
            if (trigger <= now || state.done.contains(k) || state.snoozed.contains(k))
                continue;
            offer(trigger);
            break;
        }
    }
    if (lookAgain)
        offer(now.addDays(kHorizonDays - 1));
    return best;
}

qint64 Reminders::wakeWhileShownMs(const QDateTime &now, const QDateTime &next)
{
    if (!next.isValid())
        return -1;
    return qBound(qint64(1000), now.msecsTo(next), qint64(6) * 3600 * 1000);
}

QByteArray Reminders::timerUnit(const QDateTime &at)
{
    // UTC, so a change of time zone or of hour moves nothing
    const QString when = at.toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) + QStringLiteral(" UTC");
    return QStringLiteral(
               "# Written by Raye Calendar for its next reminder; rewritten each time.\n"
               "[Unit]\n"
               "Description=Raye Calendar, next reminder\n"
               "\n"
               "[Timer]\n"
               "OnCalendar=%1\n"
               "AccuracySec=1s\n"
               // a reminder that came due while the computer was off shows at sign-in
               "Persistent=true\n"
               "Unit=raye-calendar-reminder.service\n"
               "\n"
               "[Install]\n"
               "WantedBy=timers.target\n")
        .arg(when)
        .toUtf8();
}
