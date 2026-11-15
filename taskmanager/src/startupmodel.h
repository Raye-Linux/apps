// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Startup apps page. The list and the switch are in autostart.h.

#pragma once

#include "autostart.h"

#include <QAbstractListModel>
#include <QQmlEngine>

namespace Raye::TaskManager
{

class StartupModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int count READ rowCountForQml NOTIFY countChanged)

public:
    enum Role { NameRole = Qt::UserRole + 1, IconRole, CommentRole, ExecRole, EnabledRole, ImpactRole, SystemRole, FileRole };
    enum ImpactLevel { NotMeasured, None, Low, Medium, High };
    Q_ENUM(ImpactLevel)

    explicit StartupModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCountForQml() const;

    Q_INVOKABLE void refresh();
    // false when the person's copy could not be written
    Q_INVOKABLE bool setEnabled(int row, bool enabled);

Q_SIGNALS:
    void countChanged();

private:
    struct Row {
        AutostartEntry entry;
        Impact impact = Impact::NotMeasured;
    };
    QList<Row> m_rows;
    QString m_userDir;
};

} // namespace Raye::TaskManager
