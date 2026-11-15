// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "fakesources.h"

#include <algorithm>

using namespace Qt::StringLiterals;

namespace
{
constexpr uint SourceMonitor = 1;
constexpr uint SourceVirtual = 4;
}

FakeOutputs::FakeOutputs(const QList<QSize> &sizes, uint sourceTypes, QObject *parent)
    : QAbstractListModel(parent)
{
    // as the backend builds it: real screens first, then "All screens"
    // (several screens), a virtual screen, the region
    if (sourceTypes & SourceMonitor) {
        int x = 0;
        int n = 0;
        for (const QSize &size : sizes) {
            ++n;
            m_rows.append({u"Monitor %1"_s.arg(n), u"monitor-symbolic"_s, u"DP-%1"_s.arg(n), QRect(QPoint(x, 0), size), false});
            x += size.width();
        }
        if (sizes.size() > 1) {
            m_rows.append({u"Share full Workspace"_s, u"preferences-desktop-display-randr-symbolic"_s, {}, {}, true});
        }
        m_rows.append({u"Share virtual screen"_s, u"window-duplicate-symbolic"_s, {}, {}, true});
        m_rows.append({u"Share region"_s, u"transform-crop-symbolic"_s, {}, {}, true});
    } else if (sourceTypes & SourceVirtual) {
        m_rows.append({u"Share virtual screen"_s, u"window-duplicate-symbolic"_s, {}, {}, true});
    }
}

int FakeOutputs::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QHash<int, QByteArray> FakeOutputs::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {Qt::DecorationRole, "decoration"},
        {Qt::CheckStateRole, "checked"},
        {ScreenRole, "screen"},
        {NameRole, "name"},
        {IsSyntheticRole, "isSynthetic"},
        {DescriptionRole, "description"},
        {GeometryRole, "geometry"},
    };
}

QVariant FakeOutputs::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const Row &row = m_rows.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return row.display;
    case Qt::DecorationRole:
        return row.icon;
    case Qt::CheckStateRole:
        return m_selected.contains(index.row()) ? Qt::Checked : Qt::Unchecked;
    case ScreenRole:
        return QVariant::fromValue<QObject *>(nullptr);
    case NameRole:
        return row.name;
    case IsSyntheticRole:
        return row.synthetic;
    case DescriptionRole:
        return QString();
    case GeometryRole:
        return row.geometry;
    }
    return {};
}

bool FakeOutputs::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid) || role != Qt::CheckStateRole) {
        return false;
    }
    if (value.toInt() == Qt::Checked) {
        m_selected.insert(index.row());
    } else {
        m_selected.remove(index.row());
    }
    Q_EMIT dataChanged(index, index, {role});
    Q_EMIT hasSelectionChanged();
    return true;
}

qsizetype FakeOutputs::syntheticCount() const
{
    return std::ranges::count_if(m_rows, [](const Row &row) {
        return row.synthetic;
    });
}

QList<int> FakeOutputs::selectedRows() const
{
    QList<int> rows(m_selected.begin(), m_selected.end());
    std::ranges::sort(rows);
    return rows;
}

void FakeOutputs::clearSelection()
{
    const auto selected = m_selected;
    m_selected.clear();
    for (int row : selected) {
        Q_EMIT dataChanged(index(row), index(row), {Qt::CheckStateRole});
    }
    Q_EMIT hasSelectionChanged();
}

FakeWindows::FakeWindows(int count, QObject *parent)
    : QAbstractListModel(parent)
{
    // a few apps people share, over and over
    static const QList<std::pair<QString, QString>> apps = {
        {u"Notes for the meeting - Notepad"_s, u"accessories-text-editor"_s},
        {u"Raye Settings"_s, u"preferences-system"_s},
        {u"Quarterly report.ods"_s, u"x-office-spreadsheet"_s},
        {u"Home - Files"_s, u"system-file-manager"_s},
        {u"Photos"_s, u"multimedia-photo-viewer"_s},
        {u"Terminal"_s, u"utilities-terminal"_s},
        {u"Calculator"_s, u"accessories-calculator"_s},
        {u"Slides for Monday.odp"_s, u"x-office-presentation"_s},
    };
    for (int i = 0; i < count; ++i) {
        const auto &app = apps.at(i % apps.size());
        const QString title = i < apps.size() ? app.first : u"%1 (%2)"_s.arg(app.first).arg(i / apps.size() + 1);
        m_rows.append({title, app.second, u"org.example.app%1"_s.arg(i)});
    }
}

FakeWindows::FakeWindows(const QStringList &titles, QObject *parent)
    : QAbstractListModel(parent)
{
    int i = 0;
    for (const QString &title : titles) {
        m_rows.append({title, u"application-x-executable"_s, u"org.example.app%1"_s.arg(i++)});
    }
}

int FakeWindows::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QHash<int, QByteArray> FakeWindows::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {Qt::DecorationRole, "decoration"},
        {Qt::CheckStateRole, "checked"},
        {AppId, "AppId"},
        {Uuid, "Uuid"},
    };
}

QVariant FakeWindows::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const Row &row = m_rows.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return row.title;
    case Qt::DecorationRole:
        return row.icon;
    case Qt::CheckStateRole:
        return m_selected.contains(index.row()) ? Qt::Checked : Qt::Unchecked;
    case AppId:
        return row.appId;
    case Uuid:
        return u"uuid-%1"_s.arg(index.row());
    }
    return {};
}

bool FakeWindows::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid) || role != Qt::CheckStateRole) {
        return false;
    }
    if (value.toInt() == Qt::Checked) {
        m_selected.insert(index.row());
    } else {
        m_selected.remove(index.row());
    }
    Q_EMIT dataChanged(index, index, {role});
    Q_EMIT hasSelectionChanged();
    return true;
}

QList<int> FakeWindows::selectedRows() const
{
    QList<int> rows(m_selected.begin(), m_selected.end());
    std::ranges::sort(rows);
    return rows;
}

void FakeWindows::clearSelection()
{
    const auto selected = m_selected;
    m_selected.clear();
    for (int row : selected) {
        Q_EMIT dataChanged(index(row), index(row), {Qt::CheckStateRole});
    }
    Q_EMIT hasSelectionChanged();
}
