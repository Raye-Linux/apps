// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicelist.h"

namespace Raye::TaskManager
{

int DeviceList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant DeviceList::data(const QModelIndex &index, int role) const
{
    if (role != DeviceRole || !index.isValid() || index.row() >= m_rows.size()) {
        return {};
    }
    return m_rows.at(index.row());
}

QHash<int, QByteArray> DeviceList::roleNames() const
{
    return {{DeviceRole, QByteArrayLiteral("device")}};
}

void DeviceList::setDevices(const QVariantList &devices)
{
    QList<QVariantMap> next;
    next.reserve(devices.size());
    for (const QVariant &d : devices) {
        next.append(d.toMap());
    }
    replaceRows(m_rows, next, [](const QVariantMap &d) { return d.value(QStringLiteral("key")).toString(); }, [](const QVariantMap &a, const QVariantMap &b) {
        return a == b;
    });
}

} // namespace Raye::TaskManager
