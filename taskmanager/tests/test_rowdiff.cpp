// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A reading where one process ended and one started must not reset the
// list: that rebuilds every row on screen.

#include "rowdiff.h"

#include <QSignalSpy>
#include <QTest>

using namespace Raye::TaskManager;

namespace
{
struct Item {
    QString key;
    int value = 0;
};

class Model : public DiffableListModel
{
public:
    QList<Item> rows;
    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : int(rows.size());
    }
    QVariant data(const QModelIndex &index, int) const override
    {
        return rows.value(index.row()).value;
    }
    void replace(const QList<Item> &next)
    {
        replaceRows(rows, next, [](const Item &i) { return i.key; }, [](const Item &a, const Item &b) {
            return a.value == b.value;
        });
    }
};

QList<Item> items(const QString &keys)
{
    QList<Item> out;
    for (const QChar c : keys) {
        out.append({QString(c), 0});
    }
    return out;
}

QString keys(const QList<Item> &list)
{
    QString out;
    for (const Item &i : list) {
        out += i.key;
    }
    return out;
}
}

class TestRowDiff : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void removesAndInsertsWithoutReset()
    {
        Model m;
        m.rows = items(QStringLiteral("abcde"));
        QSignalSpy reset(&m, &QAbstractItemModel::modelReset);
        QSignalSpy removed(&m, &QAbstractItemModel::rowsRemoved);
        QSignalSpy inserted(&m, &QAbstractItemModel::rowsInserted);
        m.replace(items(QStringLiteral("abxdey")));
        QCOMPARE(keys(m.rows), QStringLiteral("abxdey"));
        QCOMPARE(reset.count(), 0);
        QCOMPARE(removed.count(), 1);
        QCOMPARE(inserted.count(), 2);
    }

    void onlyChangedRowsAreSaid()
    {
        Model m;
        m.rows = items(QStringLiteral("abc"));
        QSignalSpy changed(&m, &QAbstractItemModel::dataChanged);
        QList<Item> next = items(QStringLiteral("abc"));
        next[1].value = 5;
        m.replace(next);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.first().first().value<QModelIndex>().row(), 1);
        QCOMPARE(m.rows.at(1).value, 5);
    }

    void newOrderResets()
    {
        Model m;
        m.rows = items(QStringLiteral("abc"));
        QSignalSpy reset(&m, &QAbstractItemModel::modelReset);
        m.replace(items(QStringLiteral("cab")));
        QCOMPARE(reset.count(), 1);
        QCOMPARE(keys(m.rows), QStringLiteral("cab"));
    }

    void fromAndToNothing()
    {
        Model m;
        m.replace(items(QStringLiteral("ab")));
        QCOMPARE(keys(m.rows), QStringLiteral("ab"));
        m.replace({});
        QCOMPARE(m.rows.size(), 0);
    }
};

QTEST_GUILESS_MAIN(TestRowDiff)
#include "test_rowdiff.moc"
