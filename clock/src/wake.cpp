// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wake.h"

#include "clockstore.h"

#include <QFile>
#include <QProcess>
#include <QStandardPaths>

QDateTime Wake::nextAlarm(const QList<Alarm> &alarms, const RingState &state, const QDateTime &now, const QTimeZone &zone)
{
    QDateTime best;
    for (const Alarm &a : alarms) {
        const QDateTime t = Rings::nextOf(a, state, now, zone);
        if (t.isValid() && (!best.isValid() || t < best))
            best = t;
    }
    return best;
}

qint64 Wake::wakeEpoch(const QDateTime &at, const QDateTime &now)
{
    return qMax(at.toSecsSinceEpoch() - kLeadSecs, now.toSecsSinceEpoch());
}

QByteArray Wake::timerUnit(qint64 epoch)
{
    const QString when = QDateTime::fromSecsSinceEpoch(epoch, QTimeZone::UTC).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    return QStringLiteral(
               "# Written by raye-clock-wake for the next Raye Clock alarm; rewritten each time.\n"
               "[Unit]\n"
               "Description=Wake up for the next alarm\n"
               "\n"
               "[Timer]\n"
               "OnCalendar=%1 UTC\n"
               "AccuracySec=1s\n"
               "WakeSystem=true\n"
               "Unit=raye-clock-wake.service\n")
        .arg(when)
        .toUtf8();
}

QString Wake::unitName(uint uid)
{
    return QStringLiteral("raye-clock-wake-%1.timer").arg(uid);
}

bool Wake::validEpoch(qint64 epoch, qint64 now)
{
    return epoch >= now - 60 && epoch <= now + qint64(400) * 24 * 3600;
}

uint Wake::parseUid(const QByteArray &text, bool *ok)
{
    *ok = !text.isEmpty() && text.size() <= 10;
    for (const char c : text)
        *ok = *ok && c >= '0' && c <= '9';
    return *ok ? text.toUInt(ok) : 0;
}

bool Wake::rtcCanWake(const QString &sysRoot)
{
    return QFile::exists(sysRoot + QStringLiteral("/class/rtc/rtc0/wakealarm"));
}

Wake::Helper Wake::realHelper()
{
    return [](const QStringList &args) {
        QProcess p;
        p.start(QStringLiteral("pkexec"), QStringList{QStringLiteral(RAYE_CLOCK_WAKE_HELPER)} + args);
        return p.waitForFinished(30000) && p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
    };
}

bool Wake::helperInstalled()
{
    return QFile::exists(QStringLiteral(RAYE_CLOCK_WAKE_HELPER)) && !QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty();
}

Wake::Result Wake::apply(const QDateTime &nextAlarm, const QDateTime &now, const QString &lastPath, bool canWake, const Helper &helper)
{
    if (!canWake)
        return Result::Unavailable;
    const QByteArray last = ClockStore::read(lastPath).trimmed();
    if (!nextAlarm.isValid()) {
        if (last.isEmpty())
            return Result::Unchanged;
        if (!helper({QStringLiteral("clear")}))
            return Result::Failed;
        QFile::remove(lastPath);
        return Result::Cleared;
    }
    const QByteArray epoch = QByteArray::number(wakeEpoch(nextAlarm, now));
    if (epoch == last)
        return Result::Unchanged;
    if (!helper({QStringLiteral("set"), QString::fromLatin1(epoch)}))
        return Result::Failed;
    ClockStore::writeAtomic(lastPath, epoch);
    return Result::Set;
}
