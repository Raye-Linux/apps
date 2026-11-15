// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "services.h"

#include <QFileInfo>
#include <QMap>

#include <algorithm>

namespace Raye::TaskManager
{

QString serviceState(const QString &active, const QString &sub)
{
    if (active == QLatin1String("active")) {
        return sub == QLatin1String("exited") ? QStringLiteral("finished") : QStringLiteral("running");
    }
    if (active == QLatin1String("failed")) {
        return QStringLiteral("failed");
    }
    if (active == QLatin1String("activating") || active == QLatin1String("reloading")) {
        return QStringLiteral("starting");
    }
    if (active == QLatin1String("deactivating")) {
        return QStringLiteral("stopping");
    }
    return QStringLiteral("stopped");
}

QList<ServiceRow> mergeServices(const QList<QStringList> &units, const QList<QStringList> &files)
{
    QMap<QString, ServiceRow> rows;
    for (const QStringList &f : files) {
        const QString name = QFileInfo(f.value(0)).fileName();
        const QString state = f.value(1);
        if (!name.endsWith(QLatin1String(".service")) || name.contains(QLatin1String("@.")) || state == QLatin1String("alias")) {
            continue;
        }
        ServiceRow &row = rows[name];
        row.unit = name;
        row.state = QStringLiteral("stopped");
        row.startup = state;
    }
    for (const QStringList &u : units) {
        const QString name = u.value(0);
        if (!name.endsWith(QLatin1String(".service")) || u.value(2) == QLatin1String("not-found")) {
            continue;
        }
        ServiceRow &row = rows[name];
        row.unit = name;
        row.description = u.value(1);
        row.state = serviceState(u.value(3), u.value(4));
    }
    QList<ServiceRow> out = rows.values();
    // NetworkManager among the n's, not ahead of every lower-case name
    std::stable_sort(out.begin(), out.end(), [](const ServiceRow &a, const ServiceRow &b) {
        return a.unit.compare(b.unit, Qt::CaseInsensitive) < 0;
    });
    return out;
}

QString serviceProcsFile(const QString &unit, const QString &cgroupRoot)
{
    if (unit.isEmpty() || unit.contains(QLatin1Char('/')) || unit.startsWith(QLatin1Char('.')))
        return QString();
    return cgroupRoot + QStringLiteral("/system.slice/") + unit + QStringLiteral("/cgroup.procs");
}

} // namespace Raye::TaskManager
