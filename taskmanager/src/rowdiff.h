// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Bringing a list model from one reading to the next with the smallest
// change: rows that went are removed, rows that came are inserted, and
// only the rows whose figures moved are said to have changed. A reset
// every second would rebuild every row on screen each time a process
// starts or ends somewhere, which on a busy machine is every second.

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>

namespace Raye::TaskManager
{

// The protected parts of QAbstractListModel this needs, opened up.
class DiffableListModel : public QAbstractListModel
{
public:
    using QAbstractListModel::QAbstractListModel;

protected:
    // key(row) names a row across readings; same(a, b) says whether its
    // figures are unchanged. When the rows that stay have changed their
    // order (a new sort, or figures that reorder them) it falls back on a
    // reset, which is then the honest thing to say.
    template<typename Row, typename Key, typename Same>
    void replaceRows(QList<Row> &current, const QList<Row> &next, Key key, Same same)
    {
        QHash<decltype(key(next.first())), int> wanted;
        for (int i = 0; i < next.size(); ++i) {
            wanted.insert(key(next.at(i)), i);
        }
        // the rows that stay must keep their order, else reset
        int last = -1;
        bool ordered = true;
        for (const Row &row : std::as_const(current)) {
            const auto at = wanted.constFind(key(row));
            if (at == wanted.constEnd()) {
                continue;
            }
            if (*at < last) {
                ordered = false;
                break;
            }
            last = *at;
        }
        if (!ordered) {
            beginResetModel();
            current = next;
            endResetModel();
            return;
        }
        for (int i = int(current.size()) - 1; i >= 0; --i) {
            if (!wanted.contains(key(current.at(i)))) {
                beginRemoveRows({}, i, i);
                current.removeAt(i);
                endRemoveRows();
            }
        }
        for (int i = 0; i < next.size(); ++i) {
            if (i < current.size() && key(current.at(i)) == key(next.at(i))) {
                if (!same(current.at(i), next.at(i))) {
                    current[i] = next.at(i);
                    Q_EMIT dataChanged(index(i), index(i));
                }
                continue;
            }
            beginInsertRows({}, i, i);
            current.insert(i, next.at(i));
            endInsertRows();
        }
    }
};

} // namespace Raye::TaskManager
