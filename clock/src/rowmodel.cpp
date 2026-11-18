// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rowmodel.h"

#include <QSet>

RowModel::RowModel(const QStringList &roles, QObject *parent)
    : QAbstractListModel(parent)
    , m_roles(roles)
{
}

int RowModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant RowModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    return m_rows[index.row()].value(m_roles.value(role - Qt::UserRole));
}

QHash<int, QByteArray> RowModel::roleNames() const
{
    QHash<int, QByteArray> names;
    for (int i = 0; i < m_roles.size(); ++i)
        names.insert(Qt::UserRole + i, m_roles[i].toUtf8());
    return names;
}

void RowModel::setRows(const QList<QVariantMap> &rows)
{
    const int before = int(m_rows.size());
    QSet<QString> wanted;
    for (const QVariantMap &r : rows)
        wanted.insert(r.value(QStringLiteral("key")).toString());
    // out first, from the bottom
    for (int i = int(m_rows.size()) - 1; i >= 0; --i) {
        if (!wanted.contains(m_rows[i].value(QStringLiteral("key")).toString())) {
            beginRemoveRows({}, i, i);
            m_rows.removeAt(i);
            endRemoveRows();
        }
    }
    // then walk the new order: move, insert or update in place
    for (int i = 0; i < rows.size(); ++i) {
        const QString key = rows[i].value(QStringLiteral("key")).toString();
        int at = -1;
        for (int j = i; j < m_rows.size(); ++j) {
            if (m_rows[j].value(QStringLiteral("key")).toString() == key) {
                at = j;
                break;
            }
        }
        if (at < 0) {
            beginInsertRows({}, i, i);
            m_rows.insert(i, rows[i]);
            endInsertRows();
            continue;
        }
        if (at != i) {
            beginMoveRows({}, at, at, {}, i);
            m_rows.move(at, i);
            endMoveRows();
        }
        if (m_rows[i] != rows[i]) {
            m_rows[i] = rows[i];
            Q_EMIT dataChanged(index(i), index(i));
        }
    }
    if (before != m_rows.size())
        Q_EMIT countChanged();
}
