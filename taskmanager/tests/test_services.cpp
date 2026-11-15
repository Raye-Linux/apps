// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "services.h"

#include <QTest>

using namespace Raye::TaskManager;

class TestServices : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void states()
    {
        QCOMPARE(serviceState(QStringLiteral("active"), QStringLiteral("running")), QStringLiteral("running"));
        QCOMPARE(serviceState(QStringLiteral("active"), QStringLiteral("exited")), QStringLiteral("finished"));
        QCOMPARE(serviceState(QStringLiteral("failed"), QStringLiteral("failed")), QStringLiteral("failed"));
        QCOMPARE(serviceState(QStringLiteral("activating"), QStringLiteral("start")), QStringLiteral("starting"));
        QCOMPARE(serviceState(QStringLiteral("inactive"), QStringLiteral("dead")), QStringLiteral("stopped"));
    }

    void merge()
    {
        const QList<QStringList> units = {
            {QStringLiteral("cups.service"), QStringLiteral("CUPS Scheduler"), QStringLiteral("loaded"), QStringLiteral("active"), QStringLiteral("running")},
            {QStringLiteral("gone.service"), QStringLiteral("x"), QStringLiteral("not-found"), QStringLiteral("inactive"), QStringLiteral("dead")},
            {QStringLiteral("dev-sda.device"), QStringLiteral("disk"), QStringLiteral("loaded"), QStringLiteral("active"), QStringLiteral("plugged")},
        };
        const QList<QStringList> files = {
            {QStringLiteral("/usr/lib/systemd/system/cups.service"), QStringLiteral("enabled")},
            {QStringLiteral("/usr/lib/systemd/system/ssh.service"), QStringLiteral("disabled")},
            {QStringLiteral("/usr/lib/systemd/system/getty@.service"), QStringLiteral("enabled")},
            {QStringLiteral("/usr/lib/systemd/system/sshd.service"), QStringLiteral("alias")},
            {QStringLiteral("/usr/lib/systemd/system/x.timer"), QStringLiteral("enabled")},
        };
        const QList<ServiceRow> rows = mergeServices(units, files);
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows.at(0).unit, QStringLiteral("cups.service"));
        QCOMPARE(rows.at(0).state, QStringLiteral("running"));
        QCOMPARE(rows.at(0).startup, QStringLiteral("enabled"));
        QCOMPARE(rows.at(0).description, QStringLiteral("CUPS Scheduler"));
        QCOMPARE(rows.at(1).unit, QStringLiteral("ssh.service"));
        QCOMPARE(rows.at(1).state, QStringLiteral("stopped"));
        // upper case does not jump ahead
        const QList<ServiceRow> mixed = mergeServices({}, {{QStringLiteral("b.service"), QStringLiteral("enabled")},
                                                           {QStringLiteral("NetworkManager.service"), QStringLiteral("enabled")},
                                                           {QStringLiteral("avahi.service"), QStringLiteral("enabled")}});
        QCOMPARE(mixed.at(2).unit, QStringLiteral("NetworkManager.service"));
    }

    // systemd's names are escaped already: escaping them again lost the PID
    void procsFile()
    {
        QCOMPARE(serviceProcsFile(QStringLiteral("cups-browsed.service"), QStringLiteral("/cg")),
                 QStringLiteral("/cg/system.slice/cups-browsed.service/cgroup.procs"));
        QCOMPARE(serviceProcsFile(QStringLiteral("systemd-fsck@dev-disk-by\\x2duuid-1.service"), QStringLiteral("/cg")),
                 QStringLiteral("/cg/system.slice/systemd-fsck@dev-disk-by\\x2duuid-1.service/cgroup.procs"));
        QCOMPARE(serviceProcsFile(QStringLiteral("../x.service")), QString());
    }
};

QTEST_GUILESS_MAIN(TestServices)
#include "test_services.moc"
