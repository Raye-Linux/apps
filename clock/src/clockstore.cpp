// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clockstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
const QString kTimer = QStringLiteral("raye-clock-ring.timer");
const QString kService = QStringLiteral("raye-clock-ring.service");

QString under(QStandardPaths::StandardLocation where, const QString &rest)
{
    return QStandardPaths::writableLocation(where) + QLatin1Char('/') + rest;
}
} // namespace

QString ClockStore::alarmsPath()
{
    return under(QStandardPaths::GenericDataLocation, QStringLiteral("raye-clock/alarms.json"));
}

QString ClockStore::timersPath()
{
    return under(QStandardPaths::GenericDataLocation, QStringLiteral("raye-clock/timers.json"));
}

QString ClockStore::stopwatchPath()
{
    return under(QStandardPaths::GenericDataLocation, QStringLiteral("raye-clock/stopwatch.json"));
}

QString ClockStore::statePath()
{
    return under(QStandardPaths::GenericStateLocation, QStringLiteral("raye-clock/rings.json"));
}

QString ClockStore::settingsPath()
{
    return under(QStandardPaths::GenericConfigLocation, QStringLiteral("raye/raye-clock.conf"));
}

QString ClockStore::timerPath()
{
    return under(QStandardPaths::GenericConfigLocation, QStringLiteral("systemd/user/") + kTimer);
}

bool ClockStore::writeAtomic(const QString &path, const QByteArray &data, QString *error)
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

QByteArray ClockStore::read(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

Schedule::Systemctl Schedule::realSystemctl()
{
    return [](const QStringList &args) {
        QProcess p;
        p.start(QStringLiteral("systemctl"), QStringList{QStringLiteral("--user")} + args);
        return p.waitForFinished(15000) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
    };
}

Schedule::Action Schedule::apply(const QList<Alarm> &alarms, const QList<Countdown> &timers, const RingState &state,
                                 const QDateTime &now, const QTimeZone &zone, const QString &timerPath,
                                 const Systemctl &systemctl)
{
    const QString wants = QFileInfo(timerPath).absolutePath() + QStringLiteral("/timers.target.wants/") + kTimer;

    if (!Rings::due(alarms, timers, state, now, zone).isEmpty()) {
        systemctl({QStringLiteral("start"), QStringLiteral("--no-block"), kService});
        return Action::StartNow;
    }

    const QDateTime next = Rings::next(alarms, timers, state, now, zone);
    if (!next.isValid()) {
        if (QFile::exists(timerPath)) {
            systemctl({QStringLiteral("stop"), kTimer});
            QFile::remove(wants);
            QFile::remove(timerPath);
            systemctl({QStringLiteral("daemon-reload")});
        }
        return Action::Nothing;
    }

    const QByteArray unit = Rings::timerUnit(next);
    if (ClockStore::read(timerPath) != unit) {
        if (!ClockStore::writeAtomic(timerPath, unit))
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
