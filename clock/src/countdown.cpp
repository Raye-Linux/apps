// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "countdown.h"
#include "alarms.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

qint64 Countdown::remainingMs(const QDateTime &now) const
{
    if (!running())
        return qBound(qint64(0), leftMs, durationMs);
    return qBound(qint64(0), now.msecsTo(endsAt), durationMs);
}

double Countdown::fraction(const QDateTime &now) const
{
    return durationMs > 0 ? double(remainingMs(now)) / double(durationMs) : 0.0;
}

void Countdown::start(const QDateTime &now)
{
    if (running())
        return;
    // started when over: it starts again from the top
    if (leftMs <= 0)
        leftMs = durationMs;
    endsAt = now.toUTC().addMSecs(leftMs);
}

void Countdown::pause(const QDateTime &now)
{
    if (!running())
        return;
    leftMs = remainingMs(now);
    endsAt = QDateTime();
}

void Countdown::reset()
{
    leftMs = durationMs;
    endsAt = QDateTime();
}

void Countdown::restart(const QDateTime &now)
{
    reset();
    start(now);
}

void Countdown::setDuration(qint64 ms)
{
    durationMs = qBound(qint64(1000), ms, Countdowns::kLongestMs);
    reset();
}

QByteArray Countdowns::toJson(const QList<Countdown> &timers)
{
    QJsonArray list;
    for (const Countdown &c : timers) {
        QJsonObject o{
            {QStringLiteral("id"), c.id},
            {QStringLiteral("name"), c.name},
            {QStringLiteral("duration"), c.durationMs},
            {QStringLiteral("left"), c.leftMs},
        };
        if (c.running())
            o.insert(QStringLiteral("ends"), c.endsAt.toUTC().toString(Qt::ISODateWithMs));
        list.append(o);
    }
    return QJsonDocument(QJsonObject{{QStringLiteral("timers"), list}}).toJson(QJsonDocument::Indented);
}

QList<Countdown> Countdowns::fromJson(const QByteArray &data)
{
    QList<Countdown> out;
    const QJsonArray list = QJsonDocument::fromJson(data).object().value(QStringLiteral("timers")).toArray();
    for (const QJsonValue &v : list) {
        const QJsonObject o = v.toObject();
        Countdown c;
        c.id = o.value(QStringLiteral("id")).toString();
        c.durationMs = qint64(o.value(QStringLiteral("duration")).toDouble());
        if (c.id.isEmpty() || c.durationMs <= 0)
            continue;
        c.durationMs = qMin(c.durationMs, kLongestMs);
        c.name = o.value(QStringLiteral("name")).toString();
        c.leftMs = qBound(qint64(0), qint64(o.value(QStringLiteral("left")).toDouble(double(c.durationMs))), c.durationMs);
        c.endsAt = QDateTime::fromString(o.value(QStringLiteral("ends")).toString(), Qt::ISODateWithMs);
        out << c;
    }
    return out;
}

QList<Countdown> Countdowns::starters()
{
    QList<Countdown> out;
    for (int minutes : {1, 3, 5, 10}) {
        Countdown c;
        c.id = Alarms::newId();
        c.setDuration(qint64(minutes) * 60 * 1000);
        out << c;
    }
    return out;
}

qint64 Stopwatch::elapsedMs(const QDateTime &now) const
{
    return bankedMs + (running() ? qMax(qint64(0), since.msecsTo(now)) : 0);
}

void Stopwatch::start(const QDateTime &now)
{
    if (!running())
        since = now.toUTC();
}

void Stopwatch::pause(const QDateTime &now)
{
    if (!running())
        return;
    bankedMs = elapsedMs(now);
    since = QDateTime();
}

void Stopwatch::lap(const QDateTime &now)
{
    if (running())
        laps << elapsedMs(now);
}

void Stopwatch::reset()
{
    *this = Stopwatch();
}

QList<Stopwatch::Row> Stopwatch::rows() const
{
    QList<Row> out;
    qint64 before = 0;
    for (int i = 0; i < laps.size(); ++i) {
        out.prepend({i + 1, laps[i] - before, laps[i], false, false});
        before = laps[i];
    }
    if (out.size() >= 3) {
        auto byLap = [](const Row &a, const Row &b) { return a.lapMs < b.lapMs; };
        std::min_element(out.begin(), out.end(), byLap)->fastest = true;
        std::max_element(out.begin(), out.end(), byLap)->slowest = true;
    }
    return out;
}

QByteArray Stopwatch::toJson() const
{
    QJsonArray list;
    for (qint64 l : laps)
        list.append(l);
    QJsonObject o{{QStringLiteral("banked"), bankedMs}, {QStringLiteral("laps"), list}};
    if (running())
        o.insert(QStringLiteral("since"), since.toUTC().toString(Qt::ISODateWithMs));
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

Stopwatch Stopwatch::fromJson(const QByteArray &data)
{
    const QJsonObject o = QJsonDocument::fromJson(data).object();
    Stopwatch s;
    s.bankedMs = qMax(qint64(0), qint64(o.value(QStringLiteral("banked")).toDouble()));
    s.since = QDateTime::fromString(o.value(QStringLiteral("since")).toString(), Qt::ISODateWithMs);
    for (const QJsonValue &v : o.value(QStringLiteral("laps")).toArray())
        s.laps << qint64(v.toDouble());
    return s;
}

Duration::Parts Duration::split(qint64 ms)
{
    ms = qMax(qint64(0), ms);
    Parts p;
    p.hours = ms / 3600000;
    p.minutes = int(ms / 60000 % 60);
    p.seconds = int(ms / 1000 % 60);
    p.hundredths = int(ms / 10 % 100);
    return p;
}

QString Duration::clockText(qint64 ms)
{
    const Parts p = split((qMax(qint64(0), ms) + 999) / 1000 * 1000);
    return QStringLiteral("%1:%2:%3")
        .arg(p.hours, 2, 10, QLatin1Char('0'))
        .arg(p.minutes, 2, 10, QLatin1Char('0'))
        .arg(p.seconds, 2, 10, QLatin1Char('0'));
}

QString Duration::preciseText(qint64 ms)
{
    const Parts p = split(ms);
    return QStringLiteral("%1:%2:%3.%4")
        .arg(p.hours, 2, 10, QLatin1Char('0'))
        .arg(p.minutes, 2, 10, QLatin1Char('0'))
        .arg(p.seconds, 2, 10, QLatin1Char('0'))
        .arg(p.hundredths, 2, 10, QLatin1Char('0'));
}

Duration::Wait Duration::until(const QDateTime &now, const QDateTime &at)
{
    const qint64 minutes = qMax(qint64(0), (now.secsTo(at) + 59) / 60);
    return {int(minutes / 1440), int(minutes / 60 % 24), int(minutes % 60)};
}
