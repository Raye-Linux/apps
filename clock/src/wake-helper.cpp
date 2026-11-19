// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// raye-clock-wake: run as root by pkexec for Raye Clock.
//
//   set <seconds since 1970>   wake the PC from sleep then
//   clear                      no wake for this user any more
//
// One system timer per user (the one pkexec names in PKEXEC_UID), in
// /run: after a restart the clock sets it again at sign-in.

#include "wake.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSaveFile>

#include <cstdio>

namespace
{
const QString kUnitDir = QStringLiteral("/run/systemd/system");

bool systemctl(const QStringList &args)
{
    return QProcess::execute(QStringLiteral("systemctl"), args) == 0;
}

int fail(const char *why)
{
    std::fprintf(stderr, "raye-clock-wake: %s\n", why);
    return 1;
}
} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments().mid(1);

    bool ok = false;
    const uint uid = Wake::parseUid(qgetenv("PKEXEC_UID"), &ok);
    if (!ok)
        return fail("run through pkexec");
    const QString unit = Wake::unitName(uid);
    const QString path = kUnitDir + QLatin1Char('/') + unit;

    if (args == QStringList{QStringLiteral("clear")}) {
        if (!QFile::exists(path))
            return 0;
        systemctl({QStringLiteral("stop"), unit});
        QFile::remove(path);
        return systemctl({QStringLiteral("daemon-reload")}) ? 0 : 1;
    }

    if (args.size() != 2 || args.at(0) != QLatin1String("set"))
        return fail("usage: raye-clock-wake set <epoch> | clear");
    const qint64 epoch = args.at(1).toLongLong(&ok);
    if (!ok || !Wake::validEpoch(epoch, QDateTime::currentSecsSinceEpoch()))
        return fail("that time is not one to wake up at");

    QDir().mkpath(kUnitDir);
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return fail("cannot write the timer");
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    const QByteArray data = Wake::timerUnit(epoch);
    if (f.write(data) != data.size() || !f.commit())
        return fail("cannot write the timer");
    if (!systemctl({QStringLiteral("daemon-reload")}) || !systemctl({QStringLiteral("restart"), unit}))
        return fail("systemd did not take the timer");
    return 0;
}
