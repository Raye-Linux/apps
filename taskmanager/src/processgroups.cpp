// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "processgroups.h"

namespace Raye::TaskManager
{

ProcessGroupsModel::ProcessGroupsModel(QObject *parent)
    : DiffableListModel(parent)
{
}

int ProcessGroupsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant ProcessGroupsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const DisplayRow &row = m_rows.at(index.row());
    switch (role) {
    case KindRole:
        switch (row.kind) {
        case DisplayRow::Header:
            return QStringLiteral("header");
        case DisplayRow::App:
            return QStringLiteral("app");
        case DisplayRow::Child:
            return QStringLiteral("child");
        case DisplayRow::Process:
            return QStringLiteral("process");
        }
        return {};
    case SectionRole:
        return row.section;
    case KeyRole:
        return row.key;
    case NameRole:
        return row.name;
    case IconRole:
        return row.icon;
    case PidRole:
        return row.pid;
    case CountRole:
        return row.count;
    case CpuRole:
        return row.cpu;
    case MemoryRole:
        return double(row.memory);
    case DiskRole:
        return row.disk;
    case EfficiencyRole:
        return row.efficiency;
    case StateRole:
        return QString(QLatin1Char(row.state));
    case ExpandedRole:
        return row.expanded;
    case PidsRole: {
        QVariantList pids;
        for (int pid : row.pids) {
            pids.append(pid);
        }
        return pids;
    }
    }
    return {};
}

QHash<int, QByteArray> ProcessGroupsModel::roleNames() const
{
    return {
        {KindRole, "kind"},
        {SectionRole, "section"},
        {KeyRole, "key"},
        {NameRole, "name"},
        {IconRole, "icon"},
        {PidRole, "pid"},
        {CountRole, "count"},
        {CpuRole, "cpu"},
        {MemoryRole, "memory"},
        {DiskRole, "disk"},
        {EfficiencyRole, "efficiency"},
        {StateRole, "state"},
        {ExpandedRole, "expanded"},
        {PidsRole, "pids"},
    };
}

ProcessModel *ProcessGroupsModel::source() const
{
    return m_source;
}

void ProcessGroupsModel::setSource(ProcessModel *source)
{
    if (source == m_source) {
        return;
    }
    if (m_source) {
        disconnect(m_source, nullptr, this, nullptr);
    }
    m_source = source;
    if (m_source) {
        connect(m_source, &ProcessModel::refreshed, this, &ProcessGroupsModel::rebuild);
    }
    Q_EMIT sourceChanged();
    rebuild();
}

QString ProcessGroupsModel::filter() const
{
    return m_filter;
}

void ProcessGroupsModel::setFilter(const QString &text)
{
    if (text == m_filter) {
        return;
    }
    m_filter = text;
    Q_EMIT filterChanged();
    rebuild();
}

QString ProcessGroupsModel::sortColumn() const
{
    return m_sortColumn;
}

void ProcessGroupsModel::setSortColumn(const QString &column)
{
    if (column == m_sortColumn) {
        return;
    }
    m_sortColumn = column;
    Q_EMIT sortChanged();
    rebuild();
}

bool ProcessGroupsModel::sortDescending() const
{
    return m_sortDescending;
}

void ProcessGroupsModel::setSortDescending(bool descending)
{
    if (descending == m_sortDescending) {
        return;
    }
    m_sortDescending = descending;
    Q_EMIT sortChanged();
    rebuild();
}

int ProcessGroupsModel::appCount() const
{
    for (const DisplayRow &row : m_rows) {
        if (row.kind == DisplayRow::Header && row.section == 0) {
            return row.count;
        }
    }
    return 0;
}

int ProcessGroupsModel::backgroundCount() const
{
    for (const DisplayRow &row : m_rows) {
        if (row.kind == DisplayRow::Header && row.section == 1) {
            return row.count;
        }
    }
    return 0;
}

void ProcessGroupsModel::toggle(const QString &key)
{
    if (m_expanded.contains(key)) {
        m_expanded.remove(key);
    } else {
        m_expanded.insert(key);
    }
    rebuild();
}

void ProcessGroupsModel::rebuild()
{
    SortKey key = SortKey::Name;
    if (m_sortColumn == QLatin1String("cpu")) {
        key = SortKey::Cpu;
    } else if (m_sortColumn == QLatin1String("memory")) {
        key = SortKey::Memory;
    } else if (m_sortColumn == QLatin1String("disk")) {
        key = SortKey::Disk;
    } else if (m_sortColumn == QLatin1String("pid")) {
        key = SortKey::Pid;
    }
    const QList<DisplayRow> rows = m_source ? buildDisplayRows(m_source->entries(), m_expanded, key, m_sortDescending, m_filter)
                                            : QList<DisplayRow>();
    replaceRows(m_rows, rows, [](const DisplayRow &r) { return r.key; }, [](const DisplayRow &a, const DisplayRow &b) {
        return a.cpu == b.cpu && a.memory == b.memory && a.disk == b.disk && a.count == b.count && a.state == b.state
            && a.efficiency == b.efficiency && a.name == b.name && a.icon == b.icon && a.expanded == b.expanded && a.pids == b.pids;
    });
    Q_EMIT rowsRebuilt();
}

} // namespace Raye::TaskManager
