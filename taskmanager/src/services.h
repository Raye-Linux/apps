// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The rows of the Services page, from the two answers of systemd's manager.

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace Raye::TaskManager
{

struct ServiceRow {
    QString unit; // "cups.service"
    QString description;
    QString state; // "running", "finished", "stopped", "failed", "starting", "stopping"
    QString startup; // the unit file's state: enabled, disabled, static...
    int pid = 0;
};

// What a unit is doing, in one of the words above, from its active and
// sub states. A oneshot that ran and exited is "finished", not running.
QString serviceState(const QString &active, const QString &sub);

// units is ListUnits() reduced to (name, description, load state, active
// state, sub state); files is ListUnitFiles() as (path, state). Templates
// ("x@.service") and aliases are left out: they cannot be started by that
// name. Sorted by name.
QList<ServiceRow> mergeServices(const QList<QStringList> &units, const QList<QStringList> &files);

// the file listing the processes of a system service; the name systemd
// gives is already escaped ("cups-browsed.service"), and is used as it is
QString serviceProcsFile(const QString &unit, const QString &cgroupRoot = QStringLiteral("/sys/fs/cgroup"));

} // namespace Raye::TaskManager
