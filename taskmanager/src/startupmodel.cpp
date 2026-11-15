// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "startupmodel.h"

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QStandardPaths>

#include <unistd.h>

namespace Raye::TaskManager
{

StartupModel::StartupModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_userDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/autostart"))
{
    refresh();
}

int StartupModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

int StartupModel::rowCountForQml() const
{
    return int(m_rows.size());
}

QVariant StartupModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row &row = m_rows.at(index.row());
    switch (role) {
    case NameRole:
        return row.entry.name;
    case IconRole:
        return row.entry.icon;
    case CommentRole:
        return row.entry.comment;
    case ExecRole:
        return row.entry.exec;
    case EnabledRole:
        return row.entry.enabled;
    case ImpactRole:
        return int(row.impact);
    case SystemRole:
        return row.entry.systemEntry;
    case FileRole:
        return row.entry.fileName;
    }
    return {};
}

QHash<int, QByteArray> StartupModel::roleNames() const
{
    return {{NameRole, "name"},       {IconRole, "icon"},     {CommentRole, "comment"}, {ExecRole, "exec"},
            {EnabledRole, "startsAtSignIn"}, {ImpactRole, "impact"}, {SystemRole, "system"},   {FileRole, "fileName"}};
}

void StartupModel::refresh()
{
    QStringList systemDirs;
    // XDG_CONFIG_DIRS, the first one winning; /etc/xdg when it is unset
    const QStringList configDirs = qEnvironmentVariable("XDG_CONFIG_DIRS", QStringLiteral("/etc/xdg")).split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString &dir : configDirs) {
        systemDirs.append(dir + QStringLiteral("/autostart"));
    }
    const QList<AutostartEntry> entries = listAutostart(systemDirs, m_userDir, QLocale().name(),
                                                        qEnvironmentVariable("XDG_CURRENT_DESKTOP", QStringLiteral("KDE")));
    const QString uid = QString::number(::getuid());
    const QString appSlice = QStringLiteral("/sys/fs/cgroup/user.slice/user-%1.slice/user@%1.service/app.slice/").arg(uid);

    QList<Row> rows;
    for (const AutostartEntry &e : entries) {
        Row row;
        row.entry = e;
        QFile stat(appSlice + autostartUnitName(e.fileName) + QStringLiteral("/cpu.stat"));
        qlonglong usec = -1;
        if (e.enabled && stat.open(QIODevice::ReadOnly)) {
            usec = parseCpuStatUsage(stat.readAll());
        }
        row.impact = impactForCpuUsec(usec);
        rows.append(row);
    }
    std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) {
        return a.entry.name.localeAwareCompare(b.entry.name) < 0;
    });
    const int was = int(m_rows.size());
    beginResetModel();
    m_rows = rows;
    endResetModel();
    if (was != m_rows.size()) {
        Q_EMIT countChanged();
    }
}

bool StartupModel::setEnabled(int row, bool enabled)
{
    if (row < 0 || row >= m_rows.size()) {
        return false;
    }
    if (!setAutostartEnabled(m_rows.at(row).entry, enabled, m_userDir)) {
        return false;
    }
    refresh();
    return true;
}

} // namespace Raye::TaskManager
