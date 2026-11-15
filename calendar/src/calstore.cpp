// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "calstore.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
const QString kTimer = QStringLiteral("raye-calendar-reminder.timer");
const QString kService = QStringLiteral("raye-calendar-reminder.service");

QString under(QStandardPaths::StandardLocation where, const QString &rest)
{
    return QStandardPaths::writableLocation(where) + QLatin1Char('/') + rest;
}
} // namespace

QString CalStore::calendarPath()
{
    return under(QStandardPaths::GenericDataLocation, QStringLiteral("raye-calendar/calendar.ics"));
}

QString CalStore::statePath()
{
    return under(QStandardPaths::GenericStateLocation, QStringLiteral("raye-calendar/reminders.json"));
}

QString CalStore::settingsPath()
{
    return under(QStandardPaths::GenericConfigLocation, QStringLiteral("raye/raye-calendar.conf"));
}

QString CalStore::timerPath()
{
    return under(QStandardPaths::GenericConfigLocation, QStringLiteral("systemd/user/") + kTimer);
}

namespace
{
bool changesOneOccurrence(const CalEvent &ev)
{
    for (const QString &line : ev.extra) {
        if (line.startsWith(QLatin1String("RECURRENCE-ID"), Qt::CaseInsensitive))
            return true;
    }
    return false;
}
}

int CalStore::mergeImport(CalFile &into, const CalFile &in, int *leftOut)
{
    int added = 0;
    int skipped = 0;
    for (const CalEvent &ev : in.events) {
        if (changesOneOccurrence(ev)) {
            ++skipped;
            continue;
        }
        auto same = std::find_if(into.events.begin(), into.events.end(), [&ev](const CalEvent &e) {
            return e.uid == ev.uid && !changesOneOccurrence(e);
        });
        if (same != into.events.end())
            *same = ev;
        else
            into.events << ev;
        ++added;
    }
    if (leftOut)
        *leftOut = skipped;
    return added;
}

bool CalStore::writeAtomic(const QString &path, const QByteArray &data, QString *error)
{
    const QString dir = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(dir)) {
        if (error)
            *error = QStringLiteral("cannot make %1").arg(dir);
        return false;
    }
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    if (f.write(data) != data.size() || !f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return true;
}

bool CalStore::load(const QString &path, CalFile *out, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    *out = Ical::parse(f.readAll());
    return true;
}

bool CalStore::save(const QString &path, const CalFile &file, QString *error)
{
    return writeAtomic(path, Ical::serialize(file), error);
}

CalFile CalStore::loadOrEmpty(const QString &path)
{
    CalFile file;
    load(path, &file);
    return file;
}

ReminderState CalStore::loadState(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return ReminderState::fromJson(f.readAll());
}

bool CalStore::saveState(const QString &path, const ReminderState &state)
{
    return writeAtomic(path, state.toJson());
}

Schedule::Systemctl Schedule::realSystemctl()
{
    return [](const QStringList &args) {
        QProcess p;
        p.start(QStringLiteral("systemctl"), QStringList{QStringLiteral("--user")} + args);
        return p.waitForFinished(15000) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
    };
}

Schedule::Action Schedule::apply(const QList<CalEvent> &events, const ReminderState &state, const QDateTime &now,
                                 const QString &timerPath, const Systemctl &systemctl)
{
    const QString wants = QFileInfo(timerPath).absolutePath() + QStringLiteral("/timers.target.wants/") + kTimer;

    if (!Reminders::due(events, state, now).isEmpty()) {
        systemctl({QStringLiteral("start"), QStringLiteral("--no-block"), kService});
        return Action::StartNow;
    }

    const QDateTime next = Reminders::next(events, state, now);
    if (!next.isValid()) {
        if (QFile::exists(timerPath)) {
            systemctl({QStringLiteral("stop"), kTimer});
            QFile::remove(wants);
            QFile::remove(timerPath);
            systemctl({QStringLiteral("daemon-reload")});
        }
        return Action::Nothing;
    }

    const QByteArray unit = Reminders::timerUnit(next);
    QFile old(timerPath);
    const bool same = old.open(QIODevice::ReadOnly) && old.readAll() == unit;
    old.close();
    if (!same) {
        if (!CalStore::writeAtomic(timerPath, unit))
            return Action::Nothing;
        systemctl({QStringLiteral("daemon-reload")});
    }
    // what `systemctl enable` would do, without a second round trip
    QDir().mkpath(QFileInfo(wants).absolutePath());
    if (!QFileInfo(wants).isSymLink())
        QFile::link(timerPath, wants);
    systemctl({QStringLiteral("restart"), kTimer});
    return Action::Timer;
}
