// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The disks, network adapters or graphics cards of the Performance page,
// one row each, as a list model. A reading of the same devices changes
// their rows; only a device that comes or goes adds or removes one.

#pragma once

#include "rowdiff.h"

#include <QVariantList>
#include <QVariantMap>

namespace Raye::TaskManager
{

class DeviceList : public DiffableListModel
{
    Q_OBJECT

public:
    enum Roles { DeviceRole = Qt::UserRole + 1 };

    using DiffableListModel::DiffableListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // each map has a "key" naming the device across readings
    void setDevices(const QVariantList &devices);

private:
    QList<QVariantMap> m_rows;
};

} // namespace Raye::TaskManager
