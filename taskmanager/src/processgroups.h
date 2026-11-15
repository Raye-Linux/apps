// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Processes page's rows: apps with their processes under them, then
// the background processes. Built from a ProcessModel after each of its
// readings; the grouping itself is buildDisplayRows() in appgroups.h.

#pragma once

#include "appgroups.h"
#include "processmodel.h"

#include "rowdiff.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlEngine>
#include <QSet>

namespace Raye::TaskManager
{

class ProcessGroupsModel : public DiffableListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Raye::TaskManager::ProcessModel *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    // "name", "cpu", "memory", "disk", "pid"
    Q_PROPERTY(QString sortColumn READ sortColumn WRITE setSortColumn NOTIFY sortChanged)
    Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending NOTIFY sortChanged)
    // machine totals of what is listed, for the column headings
    Q_PROPERTY(int appCount READ appCount NOTIFY rowsRebuilt)
    Q_PROPERTY(int backgroundCount READ backgroundCount NOTIFY rowsRebuilt)

public:
    enum Role {
        KindRole = Qt::UserRole + 1, // "header", "app", "child", "process"
        SectionRole,
        KeyRole,
        NameRole,
        IconRole,
        PidRole,
        CountRole,
        CpuRole,
        MemoryRole,
        DiskRole,
        EfficiencyRole,
        StateRole,
        ExpandedRole,
        PidsRole,
    };

    explicit ProcessGroupsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    ProcessModel *source() const;
    void setSource(ProcessModel *source);
    QString filter() const;
    void setFilter(const QString &text);
    QString sortColumn() const;
    void setSortColumn(const QString &column);
    bool sortDescending() const;
    void setSortDescending(bool descending);
    int appCount() const;
    int backgroundCount() const;

    Q_INVOKABLE void toggle(const QString &key);

Q_SIGNALS:
    void sourceChanged();
    void filterChanged();
    void sortChanged();
    void rowsRebuilt();

private:
    void rebuild();

    QPointer<ProcessModel> m_source;
    QList<DisplayRow> m_rows;
    QSet<QString> m_expanded;
    QString m_filter;
    QString m_sortColumn = QStringLiteral("name");
    bool m_sortDescending = false;
};

} // namespace Raye::TaskManager
