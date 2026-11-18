// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A list of rows for the cards, each a map with a "key". setRows() keeps
// the rows that stay and only inserts, removes or updates the others, so
// a card is not rebuilt (and its animation not cut) when one value moves.

#pragma once

#include <QAbstractListModel>
#include <QVariantMap>

class RowModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit RowModel(const QStringList &roles, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(const QList<QVariantMap> &rows);
    const QList<QVariantMap> &rows() const { return m_rows; }
    Q_INVOKABLE QVariantMap get(int row) const { return m_rows.value(row); }

Q_SIGNALS:
    void countChanged();

private:
    QStringList m_roles;
    QList<QVariantMap> m_rows;
};
